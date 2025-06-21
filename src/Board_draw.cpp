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

void Board::DrawBlock(Block block_on_board){//盤面上(手札以外)のブロックの描画
    block_on_board.Draw(block_on_board.GetPos(), 1.0, 0.0, 1.0);
}

void Board::BlockAnimation(Block moving_block, Point end_pos){//悩み中
    for(int y=0;y<100;y++){
        for(int x=0;x<100;x++){
            //while(System::Update())内で、1フレーム毎に描画するしかなくない?
        }
    }
}

//ブロックが手札に戻るアニメーション
//捨札に行くアニメーション