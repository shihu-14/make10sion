#include <Siv3D.hpp>
#include "Board.hpp"
using namespace std;

void Board::DrawBoard(){
    Point offset = {0,0};//暫定
    int32 cell = 50;

    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            if(board_usage[i][j] < 0){
                //
            }
        }
    }
}