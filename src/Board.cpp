#include "Battle.hpp"
#include "Board.hpp"
#include "BoardCalculationRules.hpp"
#include <cassert>
using namespace std;

void Board::BeginBattle(const GameStateRules::BoardProgress& progress) {
	m_calculationChanged = true;
	InitBoardCoordinate();
	for (int32 y = 0; y < GameStateRules::BoardProgress::Height; y++) {
		for (int32 x = 0; x < GameStateRules::BoardProgress::Width; x++) {
			board_usage[y][x] = progress.IsUnlocked({ x, y }) ? 0 : -1;
		}
	}
	board_number.fill(0);
	board_effect_back.fill(0);
	board_effect_front.fill(0);
	board_effect_committed.fill(0);
	board_content.fill('\0');
	expression_cell_usage.fill(BoardCalculationRules::ExpressionCellUsage::NonExpression);
	num_on_board.clear();
	board_multiply = board_multiply_base;
	board_multiply_effect.fill(0);
	board_off_def = { 1,1,1,0,0,0 };
	result_of_calc.fill(0);
	row_valid.fill(true);
	add_damage = 0;
	add_armor = 0;
	add_damage_by_cards = 0;
	off_count = 3;
	do_armor_raise = false;
	board_blocks.clear();
	drag_context = {};
	pending_zone_changes.clear();
}

void Board::BeginTurn() {//毎ターン開始時に呼び出してもらう
	InitBoardCoordinate();
	for (auto& usage : board_usage) if (usage > 0) usage = 0;
	Discard();
	board_blocks.clear();
	drag_context = {};
}

void Board::EndTurn() {
	// 次ターンへ持ち越す効果は，盤面のカードを取り除く前に確定する．
	CancelActiveDrag();
	BoardCalculationRules::CommitDelayedEffects(board_effect_back, board_effect_committed);
}

void Board::BeginUnlockSelection(const GameStateRules::BoardProgress& progress) {
	InitBoardCoordinate();
	for (int32 y = 0; y < GameStateRules::BoardProgress::Height; y++) {
		for (int32 x = 0; x < GameStateRules::BoardProgress::Width; x++) {
			const GameStateRules::BoardCell cell{ x, y };
			board_usage[y][x] = progress.IsUnlocked(cell) ? 0
				: (progress.IsUnlockable(cell) ? -2 : -1);
		}
	}
}

Point Board::GetBoardCellAt(Point screen_pos) const {
	return ScreenToBoardCell(screen_pos);
}

void Board::Update(int32 idx, const vector<int32>& relics, const BoardInputFrame& input, bool allow_input) {
	if (idx != 0) return;
	if (drag_context.active) {
		if (!input.focused || !allow_input) {
			RollbackDraggedBlock();
			CompleteVisualMotions();
		} else {
			assert(IsDragContextValid());
			BoardBlockState& selected = board_blocks[ResolveDragBlockIndex()];
			const Point drag_pos = input.cursor + drag_context.cursor_offset;
			if (input.rotate_pressed) RotateDraggedBlock();
			if (input.left_pressed || input.left_up) selected.block->SetPos(drag_pos.x, drag_pos.y);
			if (input.left_up || !input.left_pressed) PutBlock(input.cursor, drag_pos);
		}
	} else if (allow_input && input.focused && input.left_down) {
		const Point cell_pos = ScreenToBoardCell(input.cursor);
		if (cell_pos != Point{ -1,-1 }) TakeOutBlock(cell_pos, input.cursor);
	}
	DoRelic(relics);
	UpdateVisualMotions(input.delta_seconds);
}

void Board::DrawBoard(int32 idx, const bool show_calculation_values) const {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {

		DrawOnlyBoard();//Boardの描画

		for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {//ブロックの描画
			const BoardBlockState& state = board_blocks[i];
			if (state.block && (BattleCardRules::GetCardDrawLayer(state.lifecycle)
				== BattleCardRules::CardDrawLayer::StaticBoard)) {
				const Grid<double> cell_alphas = GetBoardCellAlphas(state);
				state.block->Draw(state.block->GetPos(), img_scale, 0.0, 1.0, &cell_alphas);
			}
		}
		if (show_calculation_values) {
			Array<int32> dy = { 10,10, 10, -10,-10,-10 };//行結果の表示位置を上下にずらす．
			for (int i = 0; i < 6; i++) {
				Point num = board_coordinate[i][6];
				num.y -= dy[i];
				const ColorF row_color = (board_off_def[i] == 1)
					? ColorF{ 1.0, 0.5, 0.5 }
					: ColorF{ 0.5, 1.0, 1.0 };
				font(result_of_calc[i]).draw(TextStyle::Outline(0.2, ColorF{ 0.0 }), 85,
					Arg::rightCenter = Vec2{ BattleLayoutRules::ResultColumnRight, num.y }, row_color);
				const double multiplier = board_multiply[i] + board_multiply_effect[i];
				font(U"×{:.1f}"_fmt(multiplier)).draw(
					TextStyle::Outline(0.2, ColorF{ 0.0 }), 42,
					Arg::leftCenter = Vec2{ BattleLayoutRules::MultiplierColumnLeft, num.y }, row_color);
			}
		}
	} else if (idx == 1) {
		DrawAddPlaceBoard();
	}
}

void Board::DrawInteractionOverlay() const {
	for (const auto& state : board_blocks) {
		if (!state.block || (BattleCardRules::GetCardDrawLayer(state.lifecycle)
			!= BattleCardRules::CardDrawLayer::ReturningOverlay)) continue;
		const double scale = (state.lifecycle == BattleCardRules::CardLifecycle::ReturningToHand)
			? 1.0 : img_scale;
		if (state.lifecycle == BattleCardRules::CardLifecycle::ReturningToBoard) {
			const Grid<double> cell_alphas = GetBoardCellAlphas(state);
			state.block->Draw(state.block->GetPos(), scale, 0.0, 1.0, &cell_alphas);
		} else {
			state.block->Draw(state.block->GetPos(), scale, 0.0, 1.0);
		}
	}

	if (drag_context.active) {
		const int32 dragged_index = ResolveDragBlockIndex();
		if (!IsBoardBlockIndexValid(dragged_index)) return;
		const BoardBlockState& dragged = board_blocks[dragged_index];
		if (dragged.block && (BattleCardRules::GetCardDrawLayer(dragged.lifecycle)
			== BattleCardRules::CardDrawLayer::DraggingOverlay)) {
			dragged.block->Draw(dragged.block->GetPos(), img_scale, 0.0, 1.0);

			const auto [width, height] = dragged.block->Size();
			const auto [card_x, card_y] = dragged.block->GetPos();
			const Vec2 card_top_right{
				card_x + width * 50.0 * img_scale / 2.0 + 32.0,
				card_y - height * 50.0 * img_scale / 2.0 - 32.0
			};
			const Vec2 rotate_icon_center = card_top_right + Vec2{ 4.0, 0.0 };
			rotate_icon.scaled(0.13 * rotate_hint_scale).rotated(Math::HalfPi).drawAt(
				rotate_icon_center, ColorF{ 1.0, 1.0, 1.0, 0.95 });
			font(U"R").drawAt(static_cast<int32>(24 * rotate_hint_scale),
				rotate_icon_center + Vec2{ 24.0, -20.0 } * rotate_hint_scale, Palette::Black);
		}
	}
}

Grid<double> Board::GetBoardCellAlphas(const BoardBlockState& state) const {
	if (!state.block) return {};
	const auto [width, height] = state.block->Size();
	Grid<double> alphas{ Size{ width, height }, 1.0 };
	if ((state.board_anchor.x < 0) || (state.board_anchor.y < 0)) return alphas;
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			const Point board_cell = state.board_anchor + Point{ x,y };
			if ((board_cell.x < 0)
				|| (static_cast<int32>(expression_cell_usage.width()) <= board_cell.x)
				|| (board_cell.y < 0)
				|| (static_cast<int32>(expression_cell_usage.height()) <= board_cell.y)) continue;
			alphas[y][x] = BoardCalculationRules::ResolveExpressionCellAlpha(
				expression_cell_usage[board_cell.y][board_cell.x]);
		}
	}
	return alphas;
}

void Board::UpdateVisualMotions(double delta_seconds) {
	for (auto& state : board_blocks) {
		if (!state.visual_motion.active || !state.block) continue;
		const bool completed = state.visual_motion.Update(delta_seconds);
		const Point position = Vec2{ state.motion_start }
			.lerp(Vec2{ state.motion_end }, state.visual_motion.Rate()).asPoint();
		state.block->SetPos(position.x, position.y);
		if (!completed) continue;
		BattleCardRules::SettleReturnLifecycle(state.lifecycle);
	}
}

void Board::CompleteVisualMotions() {
	for (auto& state : board_blocks) {
		if (!state.visual_motion.active || !state.block) continue;
		state.visual_motion.Complete();
		state.block->SetPos(state.motion_end.x, state.motion_end.y);
		BattleCardRules::SettleReturnLifecycle(state.lifecycle);
	}
}

bool Board::IsDragging() const {
	return drag_context.active;
}

std::vector<GameStateRules::CardZoneChange> Board::ConsumeZoneChanges() {
	auto changes = std::move(pending_zone_changes);
	pending_zone_changes.clear();
	return changes;
}

bool Board::CanStartHandDrag(int32 deck_index) const {
	const int32 index = FindBoardBlockIndex(deck_index);
	return IsBoardBlockIndexValid(index)
		&& BattleCardRules::CanStartCardDrag(board_blocks[index].lifecycle);
}

bool Board::ShouldDrawAsHand(int32 deck_index) const {
	const int32 index = FindBoardBlockIndex(deck_index);
	if (!IsBoardBlockIndexValid(index)) return true;
	return BattleCardRules::GetCardDrawLayer(board_blocks[index].lifecycle)
		== BattleCardRules::CardDrawLayer::StaticHand;
}

void Board::DetachCard(const int32 deck_index) {
	const int32 index = FindBoardBlockIndex(deck_index);
	if (index < 0) return;
	const bool occupancy_changed = HasBoardOccupancy(index);
	ClearBoardBlock(index);
	SetBlockRotation(index, 0);
	if (drag_context.active && (drag_context.deck_index == deck_index)) ClearDrag();
	pending_zone_changes.erase(std::remove_if(pending_zone_changes.begin(), pending_zone_changes.end(),
		[deck_index](const GameStateRules::CardZoneChange& change) {
			return change.card_id == deck_index;
		}), pending_zone_changes.end());
	board_blocks.erase(board_blocks.begin() + index);
	if (occupancy_changed) CalcRow();
}

void Board::CancelActiveDrag() {
	if (drag_context.active) RollbackDraggedBlock();
	CompleteVisualMotions();
}
