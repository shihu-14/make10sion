#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

void Board::DrawBoard(){//Boardの描画のみ
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.drawAt(board_coordinate[i][j]);
            }
        }
    }
}

void Board::BlockAnimation(Block moving_block, Point end_pos){//アニメーション. 移動速度が時間に反比例します(log的な)
    Point curr_pos = {moving_block.GetPos().first, moving_block.GetPos().second};
    if(CalcDist(end_pos, curr_pos) > 5.0){
        int32 new_x = (curr_pos.x*4 + end_pos.x)/5;
        int32 new_y = (curr_pos.y*4 + end_pos.y)/5;
        moving_block.SetPos(new_x, new_y);
    }
    else{
        moving_block.SetPos(end_pos.x, end_pos.y);
        auto itr = find(used_blocks.begin(), used_blocks.end(), moving_block);
        int32 idx = distance(used_blocks.begin(), itr);
        do_block_anim[idx] = -1;
    }
}

//ここでBoardのメソッドの大半を呼び出す. この関数は、毎フレーム呼び出してもらう
void Board::Update(int32 idx){//idx : 0:バトル中, 1:リザルト(マス解放時)
	if(idx == 0){
		if(is_block_selected){//Blockをドラッグしているとき
			//この時点で、PassBlock()が実行されている
			block.SetPos(Cursor::Pos().x, Cursor::Pos().y);

			if(!block.IsDragging()){
				PutBlock();
			}

			if(MouseR.down()){//blockの回転
				block.Rotate();
			}
		}
		else{
			Point pos = Cursor::Pos();
			if(0<= pos.x-offset.x <= cell_size*7 && 0 <= pos.y-offset.y <= cell_size*7 && MouseL.down()){//Board内でクリックされたとき
				int32 bx = (pos.x - offset.x) / cell_size;
				int32 by = (pos.y - offset.y) / cell_size;
				//TakeOutBlock(Point{bx, by})を呼び出したい
			}
		}

		//以下、描画処理
		DrawBoard();//Boardの描画

		Array<Block> Deck_board;//Deck_boardがprivateになっているので、publicにしてもらうまで暫定的に

		for(int i=0;i<used_blocks.size();i++){//盤面上のブロックの描画
            if(do_block_anim[i] == 0){
                //
            }
            else if(do_block_anim[i] == 1){
                //
            }
            else if(do_block_anim[i] == 2)
			if(do_block_anim[i] >= 0){
				if(do_block_anim[i] == 1){
					BlockAnimation(used_blocks[i], block_hand_pos[i]);
			    }
				else if(do_block_anim[i] == 2){
        			BlockAnimation(used_blocks[i], Point{100, 100});//捨て札の座標を指定
    			}
			}
			used_blocks[i].Draw(used_blocks[i].GetPos(), 1.0, 0.0, 1.0);
		}
	}

	else if(idx == 1){
		//リザルト(マス解放)
	}
}