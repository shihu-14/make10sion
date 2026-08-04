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
void Board::Update(int32 idx, vector<int32> relics, bool allow_input) {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {
		if (is_board_active) {
			if (drag_context.active) {//Blockをドラッグしているとき
				if (!allow_input || !IsDragContextValid()) {
					if (drag_context.from_board) RestoreDraggedBlockToBoard();
					else ReturnDraggedBlockToHand();
				} else {
					BoardBlockState& selected = board_blocks[drag_context.board_block_index];
					if (MouseL.pressed()) {
						const Point drag_pos = Cursor::Pos() + drag_context.cursor_offset;
						selected.block->SetPos(drag_pos.x, drag_pos.y);

						if (MouseR.down()) {//blockの回転
							selected.block->Rotate();
							selected.rotation = (selected.rotation + 1) % 4;
							drag_context.rotation_steps = (drag_context.rotation_steps + 1) % 4;
						}
					} else {
						PutBlock();
					}
				}
			} else if (allow_input) {
				if (MouseL.down()) {//ボード内でクリックされたとき
					const Point cell_pos = ScreenToBoardCell(Cursor::Pos());
					if (cell_pos != Point{ -1,-1 }) TakeOutBlock(cell_pos);
				}
			}
		}

		//レリック
		DoRelic(relics);
		relics_old = relics;

		//アニメーション
		for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
			if (board_blocks[i].animation == 1) {
				//手札へ移動するブロック
				BlockAnimation(i, board_blocks[i].hand_pos);
			} else if (board_blocks[i].animation == 2) {
				//捨札へ移動するブロック
				BlockAnimation(i, Point{ 1600, 880 });//捨て札の座標を指定
			}
		}


	} else if (idx == 1) {
		if (allow_input && MouseL.down())AddUsablePlace();
	}
}

void Board::DrawBoard(int32 idx) const {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {

		DrawOnlyBoard();//Boardの描画

		const int32 dragged_index = drag_context.active ? ResolveDragBlockIndex() : -1;
		for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {//ブロックの描画
			const BoardBlockState& state = board_blocks[i];
			if ((i != dragged_index) && (state.animation >= 0) && state.block) {
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

void Board::DrawDraggedBlock() const {
	if (!drag_context.active) return;
	const int32 dragged_index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(dragged_index)) return;
	const BoardBlockState& dragged = board_blocks[dragged_index];
	if ((dragged.animation >= 0) && dragged.block) {
		dragged.block->Draw(dragged.block->GetPos(), img_scale, 0.0, 1.0);
	}
}

bool Board::IsBusy() const {
	if (drag_context.active) return true;
	for (const auto& state : board_blocks) {
		if (state.animation > 0) return true;
	}
	return false;
}

bool Board::IsDragging() const {
	return drag_context.active;
}

bool Board::IsDraggingDeck(int32 deck_index) const {
	return drag_context.active && (drag_context.deck_index == deck_index);
}
