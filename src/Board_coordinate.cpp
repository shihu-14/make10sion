#include <Siv3D.hpp>
#include "Board.hpp"
using namespace std;

//private
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