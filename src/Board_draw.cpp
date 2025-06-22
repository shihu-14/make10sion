#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

//public function
void Board::DrawBoard(){//Boardの描画のみ

    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.drawAt(board_coordinate[i][j]);
            }
        }
    }

	//枠の描画
	int32 center_x = offset.x + cell_size*3.5;
	int32 center_y = offset.y + cell_size*3;
	board_frame_img.drawAt(Point{center_x, center_y});
}

//private function
void Board::BlockAnimation(Block moving_block, Point end_pos, int32 anim_num){//アニメーション. 移動速度が時間経過に反比例します(log的な)
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
        block_anim[idx] = -1;
		if(anim_num == 1){//手札
			moving_block.SetStat(1);
		}
		else if(anim_num == 2){//捨札
			moving_block.SetStat(-1);
		}
    }
}

void Board::DrawAddPlaceBoard() {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 7; j++) {
            if (board_usage[i][j] == 0) {
                chosed_board_img.drawAt(board_coordinate[i][j]); continue;
            }
            else if (board_usage[i][j] == -1) {
                board_img.drawAt(board_coordinate[i][j]); continue;
            }
            else if (board_usage[i][j] == -2) {
                //ここにチカチカさせる条件分岐
                chosable_board_img.drawAt(board_coordinate[i][j]); continue;
            }
        }
    }
}