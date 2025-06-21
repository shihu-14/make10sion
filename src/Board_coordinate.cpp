#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include <vector>
using namespace std;

//private variables

//private functions
Point Board::PutBlockAt(){
    double rSquared = 25.0;
    //Blockの左上のピースの中心の絶対座標
    int32 px = block.GetPiece(0,0).x+Cursor::Pos().x;
    int32 py = block.GetPiece(0,0).y+Cursor::Pos().y;
    //マス座標に変換
    int32 bx = (px-offset.x)/cell_size;
    int32 by = (py-offset.y)/cell_size;

    Point putAt = {-1, -1};
    double minDist = rSquared;

    for (int i=0;i<2;i++){
        for (int j=0;j<2;j++){
            if(bx<6 && by<5){
                double distSquared = CalcDist(board_coordinate[by+i][bx+j], Point{px, py});
                if(distSquared < minDist){
                    minDist = distSquared;
                    putAt = Point{bx+j, by+i};
                }
            }
        }
    }

    bool finish = false;
    for(int i=0;i<block.Size().second;i++){
        for(int j=0;j<block.Size().first;j++){
            char content = block.GetPiece(j,i).content;
            if(content != '$' && board_usage[putAt.y + i][putAt.x + j] != 0){
                putAt = {-1, -1};
                finish = true;
                break;
            }
        }
        if(finish){
            break;
        }
    }

    return putAt;
}

double CalcDist(Point a, Point b){
    return pow((a.x-b.x), 2)+pow((a.y-b.y), 2);
}

void Board::PutBlock(){
    //blockが離されたら
    Point putAt = PutBlockAt();
    if(putAt != Point{-1, -1}){
        /*
        for(int i=0;i<block.Size().second;i++){
            for(int j=0;j<block.Size().first;j++){
                char content = block.GetPiece(j,i).content;
                //if(content != '$'){
                    //board_usage[putAt.y + i][putAt.x + j] = blockNum;
                //}
            }
        }
        */
    }
    else{
        //手札に戻す
    }
    is_block_selected = false;
}

Array<pair<int32,int32>> Board::TakeOutBlock(){
    Array<pair<int32,int32>> blockCoords;
    for (int y=0;y<6;y++){
        for(int x=0;x<7;x++){
            if(board_usage[y][x] == blockNum){
                blockCoords.push_back({x,y});
            }
        }
    }
    return blockCoords;
}

void Board::InitBoardCoordinate(){
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            Point cord;
            cord.x = offset.x + cell_size/2 + cell_size*j;
            cord.y = offset.y + cell_size/2 + cell_size*i;
            board_coordinate[i][j] = cord;
        }
    }
}


//public
//選択されているBlockが渡される
void Board::PassBlock(const Block& selectedBlock, const vector<Block> deck) {
    block = selectedBlock;

    auto itr = find(deck.begin(), deck.end(), block);
    blockNum = distance(deck.begin(), itr) + 1;//1-indexedに変更
    is_block_selected = true;
}

//後でちゃんとかくUpdate