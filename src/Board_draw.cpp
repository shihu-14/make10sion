#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

//public function
void Board::DrawOnlyBoard() const {//Boardの描画のみ
	const int32 board_width = static_cast<int32>(board_usage.width());
	const int32 board_height = static_cast<int32>(board_usage.height());

    for(int i=0;i<board_height;i++){
        for(int j=0;j<board_width;j++){
            if(board_usage[i][j] >= 0){
                board_img.scaled(img_scale).drawAt(board_coordinate[i][j]);
            }
        }
    }

	const ColorF grid_color{ 0.15, 0.15, 0.15, 0.18 };
	for (int32 x = 0; x <= board_width; ++x) {
		const double line_x = offset.x + x * cell_size;
		Line{ line_x, static_cast<double>(offset.y),
			line_x, static_cast<double>(offset.y + board_height * cell_size) }
			.draw(1.0, grid_color);
	}
	for (int32 y = 0; y <= board_height; ++y) {
		const double line_y = offset.y + y * cell_size;
		Line{ static_cast<double>(offset.x), line_y,
			static_cast<double>(offset.x + board_width * cell_size), line_y }
			.draw(1.0, grid_color);
	}

	for (int32 i = 0; i < board_height; ++i) {
		for (int32 j = 0; j < board_width; ++j) {
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
