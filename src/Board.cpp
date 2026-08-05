#include "Battle.hpp"
#include "Board.hpp"
using namespace std;

void Board::InitAll() {//毎ターン開始時に呼び出してもらう
	InitBoardCoordinate();
	for (auto& usage : board_usage) if (usage > 0) usage = 0;
	Discard();
	board_blocks.clear();
	drag_context = {};
}

//ここでBoardのメソッドの大半を呼び出す. この関数は、毎フレーム呼び出してもらう
void Board::Update(int32 idx, vector<int32> relics, const BoardInputFrame& input, bool allow_input) {//idx : 0:バトル中, 1:リザルト(マス解放時)
	current_frame_number = input.frame_number;
	if (idx == 0) {
		if (is_board_active) {
			if (drag_context.active) {//Blockをドラッグしているとき
				if (!input.focused || !allow_input || !IsDragContextValid()) {
					RollbackDraggedBlock();
					if (!input.focused || !allow_input) CompleteVisualMotions();
				} else {
					BoardBlockState& selected = board_blocks[drag_context.board_block_index];
					const Point drag_pos = input.cursor + drag_context.cursor_offset;
					if (input.left_pressed || input.left_up) {
						selected.block->SetPos(drag_pos.x, drag_pos.y);

						if (input.left_pressed && input.right_down) {//blockの回転
							selected.block->Rotate();
							selected.rotation = (selected.rotation + 1) % 4;
							drag_context.rotation_steps = (drag_context.rotation_steps + 1) % 4;
						}
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
		}

		//レリック
		DoRelic(relics);
		relics_old = relics;
		UpdateVisualMotions(input.delta_seconds);
		AssertBoardState();
	} else if (idx == 1) {
		if (allow_input && input.focused && input.left_down) AddUsablePlace(input.cursor);
	}
}

void Board::DrawBoard(int32 idx) const {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {

		DrawOnlyBoard();//Boardの描画

		for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {//ブロックの描画
			const BoardBlockState& state = board_blocks[i];
			if (state.block && (BattleCardRules::GetCardDrawLayer(state.lifecycle)
				== BattleCardRules::CardDrawLayer::StaticBoard)) {
				state.block->Draw(state.block->GetPos(), img_scale, 0.0, 1.0);
			}
		}
		Array<int32> dy = { 10,10, 10, -10,-10,-10 };
		for (int i = 0; i < 6; i++) {
			Point num = board_coordinate[i][6];
			num.x += cell_size;
			num.y -= dy[i];
			if (board_off_def[i] == 1) {
				font(result_of_calc[i]).drawAt(TextStyle::Outline(0.2, ColorF{ 0.0 }), 85, num, ColorF{ 1.0, 0.5, 0.5 });
			}
			if (board_off_def[i] == 0) {
				font(result_of_calc[i]).drawAt(TextStyle::Outline(0.2, ColorF{ 0.0 }), 85, num, ColorF{ 0.5, 1.0, 1.0 });
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
		state.block->Draw(state.block->GetPos(), scale, 0.0, 1.0);
	}

	if (drag_context.active) {
		const int32 dragged_index = ResolveDragBlockIndex();
		if (!IsBoardBlockIndexValid(dragged_index)) return;
		const BoardBlockState& dragged = board_blocks[dragged_index];
		if (dragged.block && (BattleCardRules::GetCardDrawLayer(dragged.lifecycle)
			== BattleCardRules::CardDrawLayer::DraggingOverlay)) {
			dragged.block->Draw(dragged.block->GetPos(), img_scale, 0.0, 1.0);
		}
	}
}

void Board::UpdateVisualMotions(double delta_seconds) {
	for (auto& state : board_blocks) {
		if (!state.visual_motion.active || !state.block) continue;
		const bool completed = BattleCardRules::AdvanceVisualMotion(state.visual_motion, delta_seconds);
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

void Board::CancelActiveDrag() {
	if (drag_context.active) RollbackDraggedBlock();
	CompleteVisualMotions();
}
