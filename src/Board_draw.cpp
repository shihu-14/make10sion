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
    block_on_board.Draw(block_on_board.GetPos().first, block_on_board.GetPos().second, 1.0, 0.0, 1.0);
}