#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

//public function
void Board::DrawOnlyBoard() const {//Boardの描画のみ

    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.scaled(img_scale).drawAt(board_coordinate[i][j]);
            }
            if (board_effect_front[i][j] == 1) {
                font(U"+1").drawAt(60, board_coordinate[i][j], ColorF{ 0.2 });
            }
            else if (board_effect_front[i][j] == 2) {
                font(U"+2").drawAt(60, board_coordinate[i][j], ColorF{ 0.2 });
            }
            else if (board_effect_front[i][j] == 4) {
                font(U"+4").drawAt(60, board_coordinate[i][j], ColorF{ 0.2 });
            }
            else if (board_effect_front[i][j] == 5) {
                font(U"+5").drawAt(60, board_coordinate[i][j], ColorF{ 0.2 });
            }
        }
    }

	//枠の描画
	//int32 center_x = offset.x + cell_size*3.5;
	//int32 center_y = offset.y + cell_size*3;
	//board_frame_img.scaled(img_scale).drawAt(Point{center_x, center_y});
}

void Board::DrawAddPlaceBoard() const {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 7; j++) {
            if (board_usage[i][j] == 0) {
                chosed_board_img.scaled(img_scale).drawAt(board_coordinate[i][j]); continue;
            }
            else if (board_usage[i][j] == -1) {
                board_img.scaled(img_scale).drawAt(board_coordinate[i][j]); continue;
            }
            else if (board_usage[i][j] == -2) {
                //ここにチカチカさせる条件分岐
                chosable_board_img.scaled(img_scale).drawAt(board_coordinate[i][j]); continue;
            }
        }
    }
    //枠の描画
	//int32 center_x = offset.x + cell_size*3.5;
	//int32 center_y = offset.y + cell_size*3;
	//board_frame_img.scaled(img_scale).drawAt(Point{center_x, center_y});
}
