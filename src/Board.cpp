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
void Board::Update(int32 idx, vector<int32> relics) {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {
		if (is_board_active) {
			if (drag_context.active) {//Blockをドラッグしているとき
				if (!IsDragContextValid()) {
					ClearDrag();
					return;
				}
				BoardBlockState& selected = board_blocks[drag_context.board_block_index];
				if (MouseL.pressed()) {
					selected.block->SetPos(Cursor::Pos().x, Cursor::Pos().y);

					if (MouseR.down()) {//blockの回転
						selected.block->Rotate();
						selected.rotation = (selected.rotation + 1) % 4;
						drag_context.rotation_count = (drag_context.rotation_count + 1) % 4;
					}
				} else {
					PutBlock();
				}
			} else {
				const Point cursor_on_board = Cursor::Pos() - offset;
				const int32 board_width = static_cast<int32>(board_usage.width());
				const int32 board_height = static_cast<int32>(board_usage.height());
				if (MouseL.down()
					&& (0 <= cursor_on_board.x) && (cursor_on_board.x < board_width * cell_size)
					&& (0 <= cursor_on_board.y) && (cursor_on_board.y < board_height * cell_size)) {//ボード内でクリックされたとき
					Point cell_pos = cursor_on_board / cell_size;//ボードのどこのマスにあたるか
					TakeOutBlock(cell_pos);
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
		if (MouseL.down())AddUsablePlace();
	}
}

void Board::DrawBoard(int32 idx) const {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {

		DrawOnlyBoard();//Boardの描画

		for (const auto& state : board_blocks) {//ブロックの描画
			if ((state.animation >= 0) && state.block) {
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

bool Board::IsBusy() const {
	if (drag_context.active) return true;
	for (const auto& state : board_blocks) {
		if (state.animation > 0) return true;
	}
	return false;
}

bool Board::IsDraggingDeck(int32 deck_index) const {
	return IsDragContextValid() && (drag_context.deck_index == deck_index);
}
