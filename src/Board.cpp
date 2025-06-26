#include "Battle.hpp"
#include "Board.hpp"
using namespace std;

Board::Board() :  
	board_number(Size{ 7,6 }, 0),
	board_effect_back(Size{ 7,6 }, 0),
	board_effect_front(Size{ 7,6 }, 0),
	board_coordinate(Size{ 7,6 },Point{ 0,0 }),
	relics_old(19, 0)
{
	
}



void Board::InitAll(){//毎ターン開始時に呼び出してもらう
    InitBoardCoordinate();
    used_blocks.clear();
	block_hand_pos.clear();
	block_anim.clear();
}

//ここでBoardのメソッドの大半を呼び出す. この関数は、毎フレーム呼び出してもらう
void Board::Update(int32 idx, vector<int32> relics){//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {
		printf("Board::Update() called\n");
		if (is_board_active) {
			if (is_block_selected) {//Blockをドラッグしているとき
				//この時点で、PassBlock()が実行されている
				block.SetPos(Cursor::Pos().x, Cursor::Pos().y);
				used_blocks.back() = block;//手札のブロックを更新

				if (!block.IsDragging()) {
					PutBlock();
				}

				if (MouseR.down()) {//blockの回転
					block.Rotate();
				}
			}
			else {
				Point pos = Cursor::Pos();
				if (0 <= pos.x - offset.x <= cell_size * 7 && 0 <= pos.y - offset.y <= cell_size * 7 && MouseL.down()) {//Board内でクリックされたとき
					int32 bx = (pos.x - offset.x) / cell_size;
					int32 by = (pos.y - offset.y) / cell_size;
					TakeOutBlock(Point{ bx, by });
				}
			}
		}

		//レリック
		DoRelic(relics);
		relics_old = relics;

		//アニメーション
		for (int i = 0; i < used_blocks.size(); i++) {//手札へ移動するブロック
			if (block_anim[i] == 1) {
				BlockAnimation(used_blocks[i], block_hand_pos[i], block_anim[i]);
			}
		}
		for (int i = 0; i < used_blocks.size(); i++) {//捨札へ移動するブロック
			if (block_anim[i] == 2) {
				BlockAnimation(used_blocks[i], Point{ 1600, 880 }, block_anim[i]);//捨て札の座標を指定
			}
		}


	}
	else if(idx == 1){
		if(MouseL.down())AddUsablePlace();
	}
}

void Board::DrawBoard(int32 idx) const {//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {

		DrawOnlyBoard();//Boardの描画

		for (int i = 0; i < used_blocks.size(); i++) {//ブロックの描画
			if (block_anim[i] >= 0) {
				used_blocks[i].Draw(used_blocks[i].GetPos(), img_scale, 0.0, 1.0);
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
	}
	else if(idx == 1){
		DrawAddPlaceBoard();
	}
}
