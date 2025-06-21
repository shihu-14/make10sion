#include <Siv3D.hpp>
#include "Board.hpp"
using namespace std;

void Board::DrawBoard(){

    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.drawAt(board_coordinate[i][j]);
                //数字、演算子、記号 の描画

                //ブロック辺の描画
                if(board_usage[i][j] != board_usage[i][j+1]){
                    //
                }
                if(board_usage[i][j] != board_usage[i+1][j]){
                    //
                }

            }
        }
    }
    
    //ブロック辺の描画
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                //数字、演算子、記号 の描画
            }
        }
    }
}