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

Point Board::PutBlockAt(){//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す

    double rSquared = 25.0;//吸い込み半径(の2乗)
    
    //Blockの左上のピースの絶対座標
    Point piece_pos;
    piece_pos.x = block.GetPiece(0, 0).x+Cursor::Pos().x;
    piece_pos.y = block.GetPiece(0, 0).y+Cursor::Pos().y;

    //マスの中心同士を結んだ ボード座標' に変換
    int32 bx = (piece_pos.x - offset.x + cell_size/2) / cell_size;
    int32 by = (piece_pos.y - offset.y + cell_size/2) / cell_size;

    Point putAt = {-1, -1};
    double minDist = rSquared;

    //最寄りのマスの探索
    array<int32, 4> dx = {-1, 0, -1, 0};
    array<int32, 4> dy = {-1, -1, 0, 0};
    for(int k=0;k<4;k++){
        if((0 <= by+dy[k] < 6) && (0 <= bx+dx[k] < 7)){
            double distSquared = CalcDist(board_coordinate[by+dy[k]][bx+dy[k]], piece_pos);
            if(distSquared < minDist){
                minDist = distSquared;
                putAt = Point{bx+dx[k], by+dy[k]};
            }
        }
    }

    if(putAt == Point{-1, -1}){//まだ近くにマスが無い場合
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
        int32 new_x = offset.x + putAt.x*cell_size + cell_size/2;
        int32 new_y = offset.y + putAt.y*cell_size + cell_size/2;
        block.SetPos(new_x, new_y);
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
        block_anim[blockNum] = 0;
    }
    else{
        block.SetStat(1);
        block_anim[blockNum] = 1;
    }
    is_block_selected = false;
}

void Board::TakeOutBlock(Point pos){//クリックしたBlockをボードから外す
    int32 num = board_usage[pos.y][pos.x];

    if(num > 0){
        for (int y=0;y<6;y++){
            for(int x=0;x<7;x++){
                if(board_usage[y][x] == blockNum){
                    board_usage[y][x] = 0;
                    if(board_number[y][x] < 100){//数字マスなら
                        auto itr = find(num_on_board.begin(), num_on_board.end(), board_number[y][x]);
                        num_on_board.erase(itr);
                    }
                    board_number[y][x] = 0;
                    board_effect_back[y][x] = 0;
                }
            }
        }

        blockNum = num;
        block = used_blocks[blockNum - 1];
    }
}

void Board::InitBoardCoordinate(){//board_coordinateの初期化
    for(int i=0;i<6;i++){
        for(int j=0;j<7;j++){
            Point cord;
            cord.x = offset.x + cell_size*j + cell_size/2;
            cord.y = offset.y + cell_size*i + cell_size/2;
            board_coordinate[i][j] = cord;
        }
    }
}

void Board::DoRelic(vector<int32> relics){ //cf.) md
    if(relics[3]-relics_old[3] > 0){
        for(int i=0;i<6;i++){
            board_multiply[i] += 0.5;
        }
    }
    if(relics[10]-relics_old[10] > 0){
        for(int i=0;i<6;i++){
            if(board_off_def[i] == 0){
                board_off_def[i] = 1;
                break;
            }
        }
    }
    if(relics[11]-relics_old[11] > 0){
        for(int i=5;i>=0;i--){
            if(board_off_def[i] == 1){
                board_off_def[i] = 0;
                break;
            }
        }
    }
    if(relics[13]-relics_old[13] > 0){
        add_damage += (relics[13]-relics_old[13])*3;
    }
    if(relics[14]-relics_old[14] > 0){
        add_armor = relics[14]*3;
    }
    if(relics[15]-relics_old[15] > 0){
        //
    }
    if(relics[16]-relics_old[16] > 0){
        add_damage +=0;
    }
}

/*
//hpp内
int32 add_damage = 0;
int32 add_armor = 0;
void DoRelic(Leric relics);
//

//Update内(引数にLeric relicを追加)
DoRelic(relic)
relics_old = relics;
//

void Board::DoRelic(Relic relics){ //cf.) md
    if(relics[3]-relics_old[3] > 0){
        for(int i=0;i<6;i++){
            board_multiply[i] += 0.5;
        }
    }
    if(relics[10]-relics_old[10] > 0){
        for(int i=0;i<6;i++){
            if(board_off_def[i] == 0){
                board_off_def[i] = 1;
                break;
            }
        }
    }
    if(relics[11]-relics_old[11] > 0){
        for(int i=5;i>=0;i--){
            if(board_off_def[i] == 1){
                board_off_def[i] = 0;
                break;
            }
        }
    }
    if(relics[13]-relics_old[13] > 0){
        add_damage += (relics[13]-relics_old[13])*3;
    }
    if(relics[14]-relics_old[14] > 0){
        add_armor = relics[14]*3;
    }
    if(relics[15]-relics_old[15] > 0){
        //
    }
    if(relics[16]-relics_old[16] > 0){
        add_damage +=;
    }
}

*/

//public variables

//public　functions
void Board::PassBlock(const Block& selectedBlock, const Point hand_pos) {//選択されているBlockとその手札座標が渡される
    block = selectedBlock;

    auto itr = find(used_blocks.begin(), used_blocks.end(), block);
    if(itr != used_blocks.end()){
        used_blocks.push_back(block);
        blockNum = used_blocks.size();//1-indexed
        block_hand_pos.push_back(hand_pos);//手札の位置を記録
        block_anim.push_back(3); 
    }
    else{
        blockNum = distance(used_blocks.begin(), itr) + 1;//1-indexed
        block_anim[blockNum - 1] = 3;
    }
    is_block_selected = true;
}

/*
void InitAll();

void Board::InitAll(){
    InitBoardCoordinate();
    used_blocks.clear();
	block_hand_pos.clear();
	block_anim.clear();
}
*/