#include "Battle.hpp"
#include "Board.hpp"
using namespace std;

Board::Board() : 
	board_usage(Size{ 7,6 }, -1), 
	board_number(Size{ 7,6 }, 0),
	board_effect_back(Size{ 7,6 }, 0),
	board_effect_front(Size{ 7,6 }, 0),
	board_coordinate(Size{ 7,6 },Point{ 0,0 })
{
	
}

//InitBoardCoordinate()は別で、ターン開始時に呼び出してもらう


//ここでBoardのメソッドの大半を呼び出す. この関数は、毎フレーム呼び出してもらう
void Board::Update(int32 idx){//idx : 0:バトル中, 1:リザルト(マス解放時)
	if (idx == 0) {
		if (is_board_active) {
			if (is_block_selected) {//Blockをドラッグしているとき
				//この時点で、PassBlock()が実行されている
				block.SetPos(Cursor::Pos().x, Cursor::Pos().y);

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

		//以下、描画処理
		DrawBoard();//Boardの描画

		Array<Block> Deck_board;//Deck_boardがprivateになっているので、publicにしてもらうまで暫定的に

		for (int i = 0; i < used_blocks.size(); i++) {//ボードに配置されているブロック
			if (block_anim[i] == 0) {
				used_blocks[i].Draw(used_blocks[i].GetPos(), 1.0, 0.0, 1.0);
			}
		}
		for (int i = 0; i < used_blocks.size(); i++) {//手札へ移動するブロック
			if (block_anim[i] == 1) {
				BlockAnimation(used_blocks[i], block_hand_pos[i]);
				used_blocks[i].Draw(used_blocks[i].GetPos(), 1.0, 0.0, 1.0);
			}
		}
		for (int i = 0; i < used_blocks.size(); i++) {//捨札へ移動するブロック
			if (block_anim[i] == 2) {
				BlockAnimation(used_blocks[i], Point{ 100, 100 });//捨て札の座標を指定
				used_blocks[i].Draw(used_blocks[i].GetPos(), 1.0, 0.0, 1.0);
			}
		}
		for (int i = 0; i < used_blocks.size(); i++) {//その他(ホールドされているブロックだけの筈)
			if (block_anim[i] == 3) {
				used_blocks[i].Draw(used_blocks[i].GetPos(), 1.0, 0.0, 1.0);
			}
		}
	}
		//
else if(idx == 1){
	}
}