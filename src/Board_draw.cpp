#include <Siv3D.hpp>
#include "Board.hpp"
#include <array>
using namespace std;

void Board::DrawBoard(){

    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] >= 0){
                board_img.drawAt(board_coordinate[i][j]);
                //数字、演算子、記号 の描画

                //ブロック辺の描画
                array<int32, 4> dx = {0, 0, 1, -1};
                array<int32, 4> dy = {-1, 1, 0, 0};
                for(int k=0;k<4;k++){
                    if(!(0<i<5 && 0<j<6) || (board_usage[i][j] != board_usage[i + dy[k]][j + dx[k]])){//一時的なアンロック非対応
                        block_sides[k].drawAt(board_coordinate[i][j]);
                    }
                }

            }
        }
    }

    if(is_block_selected){
        //Blockの左上のピースの絶対座標
        int32 px = block.GetPiece(0,0).x+Cursor::Pos().x;
        int32 py = block.GetPiece(0,0).y+Cursor::Pos().y;

        for(int i=0;i<block.Size().second;i++){
            for(int j=0;j<block.Size().first;j++){
                char content = block.GetPiece(j,i).content;
                if(content != '$'){
                    //ピースを描画
                    //ブロック辺も?
                }
            }
        }
    }
}