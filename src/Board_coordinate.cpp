#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include <array>
#include <vector>
using namespace std;

//private variables

//private functions
Point Board::PutBlockAt(){//blockの置ける場所を確認
    double rSquared = 25.0;
    //Blockの左上のピースの中心の絶対座標
    int32 px = block.GetPiece(0,0).x+Cursor::Pos().x;
    int32 py = block.GetPiece(0,0).y+Cursor::Pos().y;
    //マスの中心同士を結んだ マス座標 に変換
    int32 bx = (px-offset.x+cell_size/2)/cell_size;
    int32 by = (py-offset.y+cell_size/2)/cell_size;

    Point putAt = {-1, -1};
    double minDist = rSquared;

    //最寄りのマスの探索
    array<int32, 4> dx = {-1, 0, -1, 0};
    array<int32, 4> dy = {-1, -1, 0, 0};
    for(int k=0;k<4;k++){
        if((0 <= by+dy[k] < 6) && (0 <= bx+dx[k] < 7)){
            double distSquared = CalcDist(board_coordinate[by+dy[k]][bx+dy[k]], Point{px, py});
            if(distSquared < minDist){
                minDist = distSquared;
                putAt = Point{bx+dx[k], by+dy[k]};
            }
        }
    }

    //置けるかどうかの確認
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

double CalcDist(Point a, Point b){//2点間の距離の計算
    return pow((a.x-b.x), 2)+pow((a.y-b.y), 2);
}

void Board::PutBlock(){//blockを配置/手札に戻す
    //blockが離されたら、という前提
    Point putAt = PutBlockAt();
    if(putAt != Point{-1, -1}){
        UpdateBoardNum(putAt);
    }
    else{
        //手札に戻す
    }
    is_block_selected = false;
}

Array<pair<int32,int32>> Board::TakeOutBlock(){//現在触っているBlockの座標を返す
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

void Board::InitBoardCoordinate(){//board_coordinateの初期化
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            Point cord;
            cord.x = offset.x + cell_size/2 + cell_size*j;
            cord.y = offset.y + cell_size/2 + cell_size*i;
            board_coordinate[i][j] = cord;
        }
    }
}


//public　functions
void Board::PassBlock(const Block& selectedBlock, const vector<Block> deck) {//選択されているBlockが渡される
    block = selectedBlock;

    auto itr = find(deck.begin(), deck.end(), block);
    blockNum = distance(deck.begin(), itr) + 1;//1-indexedに変更
    is_block_selected = true;
}

//Update()後でちゃんとかく