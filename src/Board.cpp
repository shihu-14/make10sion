#include "Battle.hpp"
#include "Board.hpp"
using namespace std;

Board::Board() : 
	board_usage(Size{ 7,6 }, -1), 
	board_number(Size{ 7,6 }, 0),
	board_effect(Size{ 7,6 }, 0),
	board_coordinate(Size{ 7,6 },Point{ 0,0 })
{
	
}

//InitBoardCoordinate()は別で、ターン開始時に呼び出してもらう

void Board::Update(int32 idx){//idx : 0:バトル中, 1:リザルト(マス解放時)
	//ここでBoardのメソッドを呼び出す
	if(idx == 0){
		if(is_block_selected){//Blockをドラッグしているとき
			//この時点で、PassBlock()が実行されている
			block.SetPos(Cursor::Pos().x, Cursor::Pos().y);
			PutBlock();
		}
		else{}
		
		//アニメーション処理
		

		//以下、描画処理
		DrawBoard();//Boardの描画

		Array<Block> Deck_board;//Deck_boardがprivateになっているので、publicにしてもらうまで暫定的に

		for(Block b: Deck_board){//盤面上のブロックの描画(わざわざ関数にすることないかも)
			DrawBlock(b);
		}
	}
	else if(idx == 1){
		//
	}
}