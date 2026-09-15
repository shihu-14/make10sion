#include "Battle.hpp"
#include "Board.hpp"
#include "BoardCalculationRules.hpp"
using namespace std;

void Board::BeginBattle(const GameStateRules::BoardProgress& progress) {
	// BattleProgressを盤面占有状態へ展開し，前回の操作状態を破棄する．
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
	interaction_trace.clear();
	pending_zone_changes.clear();
}

void Board::BeginTurn() {//毎ターン開始時に呼び出してもらう
	// 前ターンの盤面カードを次ターンの操作対象へ戻す．
	InitBoardCoordinate();
	for (auto& usage : board_usage) if (usage > 0) usage = 0;
	Discard();
	board_blocks.clear();
	drag_context = {};
}

void Board::EndTurn() {
	// Battle側のターン切り替え前に，ドラッグと遅延効果の境界を確定する．
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

//ここでBoardのメソッドの大半を呼び出す. この関数は、毎フレーム呼び出してもらう
void Board::Update(int32 idx, vector<int32> relics, const BoardInputFrame& input, bool allow_input) {//idx : 0:バトル中, 1:リザルト(マス解放時)
	// Battleから渡された入力を盤面状態へ反映し，派生計算を更新する．
	current_frame_number = input.frame_number;
	if (idx == 0) {
			if (drag_context.active) {//Blockをドラッグしているとき
				if (!input.focused || !allow_input || !IsDragContextValid()) {
					RollbackDraggedBlock();
					if (!input.focused || !allow_input) CompleteVisualMotions();
				} else {
					BoardBlockState& selected = board_blocks[drag_context.board_block_index];
					const Point drag_pos = input.cursor + drag_context.cursor_offset;
					if (BattleCardRules::ShouldRotateDraggedCard(
						drag_context.active, input.rotate_pressed)) RotateDraggedBlock();
					if (input.left_pressed || input.left_up) {
						selected.block->SetPos(drag_pos.x, drag_pos.y);
					}
					if (input.left_up) {
						PutBlock(input.cursor, drag_pos);
					} else if (!input.left_pressed) {
						PutBlock(input.cursor, drag_pos);
					}
				}
			} else if (allow_input && input.focused) {
				if (input.left_down) {//ボード内でクリックされたとき
					const Point cell_pos = ScreenToBoardCell(input.cursor);
					if (cell_pos != Point{ -1,-1 }) TakeOutBlock(cell_pos, input.cursor);
				}
			}

		//レリック
		DoRelic(relics);
		UpdateVisualMotions(input.delta_seconds);
		AssertBoardState();
	}
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
				const double multiplier = BoardCalculationRules::FinalRowMultiplier(
					board_multiply[i], board_multiply_effect[i]);
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

			// ドラッグ中のカードに，回転操作の入力方法を示す．
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
		const bool completed = BattleCardRules::AdvanceVisualMotion(
			state.visual_motion, delta_seconds,
			BattleCardRules::ResolveReturnMotionEasing(state.lifecycle));
		state.block->SetPos(state.visual_motion.current.x, state.visual_motion.current.y);
		if (!completed) continue;
		BattleCardRules::SettleReturnLifecycle(state.lifecycle);
		TraceTransition(U"motion-complete", state.deck_index);
	}
}

void Board::CompleteVisualMotions() {
	for (auto& state : board_blocks) {
		if (!state.visual_motion.active || !state.block) continue;
		BattleCardRules::CompleteVisualMotion(state.visual_motion);
		state.block->SetPos(state.visual_motion.end.x, state.visual_motion.end.y);
		BattleCardRules::SettleReturnLifecycle(state.lifecycle);
		TraceTransition(U"motion-forced-complete", state.deck_index);
	}
	AssertBoardState();
}

bool Board::IsDragging() const {
	return drag_context.active;
}

std::vector<GameStateRules::CardZoneChange> Board::ConsumeZoneChanges() {
	// 盤面側で発生したカード領域変更をバトル側へ渡す．
	auto changes = std::move(pending_zone_changes);
	pending_zone_changes.clear();
	return changes;
}

bool Board::CanStartHandDrag(int32 deck_index) const {
	const int32 index = FindBoardBlockIndex(deck_index);
	return IsBoardBlockIndexValid(index)
		&& (board_blocks[index].lifecycle == BattleCardRules::CardLifecycle::InHand)
		&& !board_blocks[index].visual_motion.active;
}

bool Board::ShouldDrawAsHand(int32 deck_index) const {
	const int32 index = FindBoardBlockIndex(deck_index);
	if (!IsBoardBlockIndexValid(index)) return true;
	return BattleCardRules::GetCardDrawLayer(board_blocks[index].lifecycle)
		== BattleCardRules::CardDrawLayer::StaticHand;
}

bool Board::DetachCard(const int32 deck_index) {
	// Battleのカード領域遷移に先行して，Board側の占有を解除する．
	const int32 index = FindBoardBlockIndex(deck_index);
	if (!IsBoardBlockIndexValid(index)) return true;
	const bool occupancy_changed = HasBoardOccupancy(index);
	if (!ClearBoardBlock(index)) return false;
	if (!BattleCardRules::CanResetCardRotation(HasBoardOccupancy(index))) return false;
	SetBlockRotation(index, 0);
	if (drag_context.active && (drag_context.deck_index == deck_index)) ClearDrag();
	pending_zone_changes.erase(std::remove_if(pending_zone_changes.begin(), pending_zone_changes.end(),
		[deck_index](const GameStateRules::CardZoneChange& change) {
			return change.card_id == deck_index;
		}), pending_zone_changes.end());
	TraceTransition(U"detach-card", deck_index);
	board_blocks.erase(board_blocks.begin() + index);
	if (occupancy_changed) CalcRow();
	AssertBoardState();
	return true;
}

void Board::CancelActiveDrag() {
	// 操作不能になったドラッグを安全に復元する．
	if (drag_context.active) RollbackDraggedBlock();
	CompleteVisualMotions();
}
