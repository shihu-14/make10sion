#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include <array>
#include <vector>
using namespace std;

//private variables
Point Board::PutBlockAt() {//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す

    double minDist = 10000.0;//吸い込み半径(の2乗)

    //Blockの左上のピースの絶対座標
    Point piece_pos;
    piece_pos.x = Cursor::Pos().x + used_blocks.at(block_number)->GetPiece(0, 0).x;
    piece_pos.y = Cursor::Pos().y + used_blocks.at(block_number)->GetPiece(0, 0).y;

    //マスの中心同士を結んだ ボード座標' に変換　ボード座標' := マス(i, j)の左上の頂点を含む領域が座標(i, j)となる
    int32 cell_x = (piece_pos.x - offset.x + cell_size / 2) / cell_size;
    int32 cell_y = (piece_pos.y - offset.y + cell_size / 2) / cell_size;

    Point putAt = { -1, -1 };

    //最寄りのマスの探索
    array<int32, 4> dx = { -1, 0, -1, 0 };
    array<int32, 4> dy = { -1, -1, 0, 0 };
    for (int k = 0;k < 4;k++) {
        if (((0 <= cell_y + dy[k]) && (cell_y + dy[k] < 6)) && ((0 <= cell_x + dx[k]) && (cell_x + dx[k] < 7))) {
            double distSquared = CalcDist(board_coordinate[cell_y + dy[k]][cell_x + dx[k]], piece_pos);
            if (distSquared < minDist) {
                minDist = distSquared;
                putAt = Point{ cell_x + dx[k], cell_y + dy[k] };
            }
        }
    }

    if (putAt == Point{ -1, -1 }) //まだ近くにマスが無い場合
        return putAt;

    //置けるかどうかの確認
    bool finish = false;
    for (int i = 0;i < used_blocks.at(block_number)->Size().second;i++) {
        for (int j = 0;j < used_blocks.at(block_number)->Size().first;j++) {
            char content = used_blocks.at(block_number)->GetPiece(j, i).content;
            if ((content != '$') && (board_usage[putAt.y + i][putAt.x + j] != 0)) {
                putAt = { -1, -1 };
                finish = true;
                break;
            }
        }
        if (finish) {
            break;
        }
    }
    if (!finish) {//吸い込まれる
        //FIXME: 回転時に座標がずれている
        int32 new_x = offset.x + putAt.x * cell_size + cell_size / 2 - used_blocks.at(block_number)->GetPiece(0, 0).x;
        int32 new_y = offset.y + putAt.y * cell_size + cell_size / 2 - used_blocks.at(block_number)->GetPiece(0, 0).y;
        used_blocks.at(block_number)->SetPos(new_x, new_y);
    }

    return putAt;

}

void Board::PutBlock() {//blockがドロップされたら、配置/手札に戻す
    Point putAt = PutBlockAt();
    if (putAt != Point{ -1, -1 }) {
        UpdateBoardNum(putAt);
        //int32 newx = offset.x + putAt.x * cell_size + cell_size/2 - block.GetPiece(0,0).x;//既に吸い込んであるから、不要かな
        //int32 newy = offset.y + putAt.y * cell_size + cell_size/2 - block.GetPiece(0,0).y;
        //block.SetPos(newx, newy);
        block_anim[blockNum - 1] = 0;
    } else {
        block_anim[blockNum - 1] = 1;
    }

    is_block_selected = false;
}

void Board::TakeOutBlock(Point pos) {//クリックしたBlockをボードから外す

    int32 num = board_usage[pos.y][pos.x];

    if (num > 0) {
        for (int y = 0;y < 6;y++) {
            for (int x = 0;x < 7;x++) {
                if (board_usage[y][x] == num) {//同じブロックのマスなら
                    if (board_number[y][x] < 100) {//数字マスなら
                        auto itr = find(num_on_board.begin(), num_on_board.end(), board_number[y][x]);
                        //FIXME: ここで範囲外アクセスが発生している！
                        num_on_board.erase(itr);
                    }
                    //防御、攻撃マスが含まれているときの処理
                    else if (board_number[y][x] == 16777217) {//攻
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < 7; i++) {
                                if (board_number[y][i] == 16777217)return false;
                            }return true;
                            }())board_off_def[y] = 0;
                    } else if (board_number[y][x] == 16777218) {//防
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < 7; i++) {
                                if (board_number[y][i] == 16777218)return false;
                            }return true;
                            }())board_off_def[y] = 1;
                    }

                    board_usage[y][x] = 0;
                    board_number[y][x] = 0;
                    board_effect_back[y][x] = 0;
                }
            }
        }

        CalcRow();

        //block = *used_blocks[num - 1];
        blockNum = num;
        is_block_selected = true;
    }
}

void Board::InitBoardCoordinate() {//board_coordinateの初期化
    for (int i = 0;i < 6;i++) {
        for (int j = 0;j < 7;j++) {
            Point cord;
            cord.x = offset.x + cell_size * j + cell_size / 2;
            cord.y = offset.y + cell_size * i + cell_size / 4;
            board_coordinate[i][j] = cord;
        }
    }
}

void Board::DoRelic(vector<int32> relics) { //cf.) md
    if (relics[3] > relics_old[3]) {
        for (int i = 0;i < 6;i++) {
            board_multiply[i] += 0.5;
        }
    }
    off_count = 3 + relics[10] - relics[11];//攻防の範囲の動かす数を記録
    if (relics[13] > relics_old[13]) {
        add_damage += (relics[13] - relics_old[13]) * 3;
    }
    if (relics[14] > relics_old[14]) {
        add_armor = relics[14] * 3;
    }

    do_armor_raise = (relics[15] == 1);

    if (relics[16] > relics_old[16]) {
        add_damage_by_cards = relics[16];
    }
}



//public variables

//public　functions
void Board::PassBlock(Block& selectedBlock, const Point hand_pos) {//選択されているBlockとその手札座標が渡される
    auto itr = find(used_blocks.begin(), used_blocks.end(), &selectedBlock);
    if (itr == used_blocks.end()) {//新出のブロックなら
        used_blocks.push_back(&selectedBlock);
        blockNum = (int)used_blocks.size();//1-indexed
        block_hand_pos.push_back(hand_pos);//手札の位置を記録
        block_anim.push_back(3);
        block_number = used_blocks.size() - 1; //ブロックの番号を更新
    } else {//既出のブロックなら
        blockNum = distance(used_blocks.begin(), itr) + 1;//1-indexed
        block_anim[blockNum - 1] = 3;
        block_number = blockNum - 1; //ブロックの番号を更新
    }
    is_block_selected = true;
    is_board_active = true; // Boardをアクティブにする
}
