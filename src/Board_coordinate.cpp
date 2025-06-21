#include <Siv3D.hpp>
#include "Board.hpp"
using namespace std;

//private
bool Board::CanPutBlock(){
    Point offset = {0,0};
    int32 cell = 32;
    double r = 25.0;
    const Point pos = Cursor::Pos();
    int32 px0 = block.GetPiece(0,0).x;
    int32 py0 = block.GetPiece(0,0).y;
    int32 px = pos.x+px0+cell/2;//Blockの左上の絶対座標
    int32 py = pos.y+py0+cell/2;

    int32 bx = (px-offset.x)/cell;//マス座標に変換
    int32 by = (py-offset.y)/cell;

    Point c = board_coordinate[by][bx];

    if(pow((c.x-px), 2)+pow((c.y-py), 2) <= r){
        return true;
    }
    return false;
}

void Board::PutBlock(){
    //
}

Array<pair<int32,int32>> Board::TakeOutBlock(){
    Array<int32> blockList(10);//Blockdeckのリスト(暫定、後で置き換える)
    auto itr = find(blockList.begin(), blockList.end(), block);
    int num = distance(blockList.begin(), itr);

    Array<pair<int32,int32>> blockCoords;
    for (int y=0;y<6;y++){
        for(int x=0;x<7;x++){
            if(board_usage[y][x] == num){
                blockCoords.push_back({x,y});
            }
        }
    }
    return blockCoords;
}

//public
//選択されているBlockが渡される
void Board::PassBlock(const Block& selectedBlock){
    block = selectedBlock;
}

//後でちゃんとかくUpdate