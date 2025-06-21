#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include <array>
#include <vector>
using namespace std;

//private variables

//private functions
double Board::CalcDist(Point a, Point b){//2点間の距離(の2乗)の計算
    return pow((a.x-b.x), 2)+pow((a.y-b.y), 2);
}

Point Board::PutBlockAt(){//blockの置ける場所を確認. blockの(0, 0)のピースのマス座標を返す
    double rSquared = 25.0;
    //Blockの左上のピースの絶対座標
    int32 px = block.GetPiece(0, 0).x+Cursor::Pos().x;
    int32 py = block.GetPiece(0, 0).y+Cursor::Pos().y;
    //マスの中心同士を結んだ マス座標 に変換
    int32 bx = (px - offset.x + cell_size/2)/cell_size;
    int32 by = (py - offset.y + cell_size/2)/cell_size;

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
    if(putAt == Point{-1, -1}){
        return putAt;
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
    if(!finish){//吸い込まれる
        int32 newx = offset.x + putAt.x*cell_size + cell_size/2;
        int32 newy = offset.y + putAt.y*cell_size + cell_size/2;
        block.SetPos(newx, newy);
    }

    return putAt;
}

void Board::PutBlock(){//blockがドロップされたら、配置/手札に戻す
    Point putAt = PutBlockAt();
    if(putAt != Point{-1, -1}){
        UpdateBoardNum(putAt);
        int32 newx = offset.x + putAt.x*cell_size + cell_size/2 + block.GetPiece(0,0).x;
        int32 newy = offset.y + putAt.y*cell_size + cell_size/2 + block.GetPiece(0,0).y;
        block.SetPos(newx, newy);
    }
    else{
        block.SetStat(1);
        do_block_anim[blockNum] = 1;
    }
    is_block_selected = false;
}

Array<pair<int32,int32>> Board::TakeOutBlock(Point pos){//現在触っているBlockの座標を返す もう少し詰めたい
    int32 num = board_usage[pos.y][pos.x];
    Array<pair<int32,int32>> blockCoords;

    if(num > 0){
        for (int y=0;y<6;y++){
            for(int x=0;x<7;x++){
                if(board_usage[y][x] == blockNum){
                    blockCoords.push_back({x,y});
                }
            }
        }
        block = used_blocks[num - 1];
        blockNum = num;
        is_block_selected = true;
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
void Board::PassBlock(const Block& selectedBlock, const Point hand_pos) {//選択されているBlockとその手札座標が渡される
    block = selectedBlock;

    auto itr = find(used_blocks.begin(), used_blocks.end(), block);
    if(itr != used_blocks.end()){
        used_blocks.push_back(block);
        blockNum = used_blocks.size();//1-indexed
        block_hand_pos.push_back(hand_pos);//手札の位置を記録
        do_block_anim.push_back(0); 
    }
    else{
        blockNum = distance(used_blocks.begin(), itr) + 1;//1-indexed
        do_block_anim[blockNum - 1] = 0;
    }
    is_block_selected = true;
}

//toアリスくん : ResetBoard()でused_blocksを初期化
//toアリスくん : ResetBoard()でdo_usedを初期化
//toアリスくん : ResetBoard()でblock_count = 1;に