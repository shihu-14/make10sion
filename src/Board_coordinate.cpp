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
//XXX:吸い込み失敗！
Point Board::PutBlockAt(){//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す

    double rSquared = 10000.0;//吸い込み半径(の2乗)
    
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
            if((content != '$') && (board_usage[putAt.y + i][putAt.x + j] != 0)){
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
        int32 new_y = offset.y + putAt.y*cell_size + cell_size;
        block.SetPos(new_x, new_y);
        used_blocks.back() = block;//手札のブロックを更新
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
        block_anim[blockNum-1] = 0;
    }
    else{
        block_anim[blockNum-1] = 1;
    }

    is_block_selected = false;
}

void Board::TakeOutBlock(Point pos){//クリックしたBlockをボードから外す

    int32 num = board_usage[pos.y][pos.x];

    if(num > 0){
        for (int y=0;y<6;y++){
            for(int x=0;x<7;x++){
                if(board_usage[y][x] == num){
                    board_usage[y][x] = 0;
                    if(board_number[y][x] < 100){//数字マスなら
                        auto itr = find(num_on_board.begin(), num_on_board.end(), board_number[y][x]);
                        num_on_board.erase(itr);
                    }
                    //防御、攻撃マスが含まれているときの処理
                    if (board_number[y][x] == 16777217) {//攻
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < 7; i++) {
                                if (board_number[y][i] == 16777217)return false;
                            }return true;
                            }())board_off_def[y] = 0;
                    }
                    if (board_number[y][x] == 16777218) {//防
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < 7; i++) {
                                if (board_number[y][i] == 16777218)return false;
                            }return true;
							}())board_off_def[y] = 1;
                    }
                    board_number[y][x] = 0;
                    board_effect_back[y][x] = 0;
                }
            }
        }

        CalcRow();

        block = used_blocks.back();
        blockNum = num;
        is_block_selected = true;
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
    if(relics[3] > relics_old[3]){
        for(int i=0;i<6;i++){
            board_multiply[i] += 0.5;
        }
    }
    off_count = 3 + relics[10] - relics[11];//攻防の範囲の動かす数を記録
    if(relics[13] > relics_old[13]){
        add_damage += (relics[13]-relics_old[13])*3;
    }
    if(relics[14] > relics_old[14]){
        add_armor = relics[14]*3;
    }

    do_armor_raise = (relics[15] == 1);

    if(relics[16] > relics_old[16]){
        add_damage_by_cards = relics[16];
    }
}



//public variables

//public　functions
void Board::PassBlock(Block& selectedBlock, const Point hand_pos) {//選択されているBlockとその手札座標が渡される
    block = selectedBlock;

    auto itr = find(used_blocks.begin(), used_blocks.end(), block);
    if(itr == used_blocks.end()){
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
    is_board_active = true; // Boardをアクティブにする
}
