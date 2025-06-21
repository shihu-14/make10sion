#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

void Board::DrawBoard(){
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.drawAt(board_coordinate[i][j]);
            }
        }
    }
    //ブロックが手札に戻るアニメーション
    //捨札に行くアニメーション
}

void Board::DrawBlockOnBoard(Block block_on_board){//vector<Block> Deck_boardから.
    block_on_board.Draw(block_on_board.GetPos(), 1.0, 0.0, 1.0);
}

void Board::BlockAnimation(Block moving_block, Point end_pos){
    for(int y=0;y<100;y++){
        for(int x=0;x<100;x++){
            //while(System::Update())内で、1フレーム毎に描画するしかなくない?
        }
    }
}