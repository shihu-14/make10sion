#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include <array>
#include <vector>
using namespace std;

//private variables
Point Board::PutBlockAt() {//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す

    double minDist = 10000.0;//吸い込み半径(の2乗)
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());

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
        if (((0 <= cell_y + dy[k]) && (cell_y + dy[k] < board_height)) && ((0 <= cell_x + dx[k]) && (cell_x + dx[k] < board_width))) {
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
            if (content == '$') continue;
            const int32 board_x = putAt.x + j;
            const int32 board_y = putAt.y + i;
            if ((board_x < 0) || (board_width <= board_x)
                || (board_y < 0) || (board_height <= board_y)
                || (board_usage[board_y][board_x] != 0)) {
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
        block_anim[block_number] = 0;
    } else {
        const int32 rotations_to_restore = (4 - block_rotation_count) % 4;
        for (int32 i = 0; i < rotations_to_restore; i++) {
            used_blocks.at(block_number)->Rotate();
        }
        if (was_block_on_board) {
            int32 new_x = offset.x + original_put_at.x * cell_size + cell_size / 2 - used_blocks.at(block_number)->GetPiece(0, 0).x;
            int32 new_y = offset.y + original_put_at.y * cell_size + cell_size / 2 - used_blocks.at(block_number)->GetPiece(0, 0).y;
            used_blocks.at(block_number)->SetPos(new_x, new_y);
            UpdateBoardNum(original_put_at);
            block_anim[block_number] = 0;
        } else {
            block_anim[block_number] = 1;
        }
    }

    is_block_selected = false;
    original_put_at = { -1,-1 };
    block_rotation_count = 0;
    was_block_on_board = false;
}

void Board::TakeOutBlock(Point pos) {//クリックしたBlockをボードから外す
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    if ((pos.x < 0) || (board_width <= pos.x) || (pos.y < 0) || (board_height <= pos.y)) return;

    int32 num = board_usage[pos.y][pos.x];

    if (num > 0) {
        const int32 selected_block_number = num - 1;
        if ((selected_block_number < 0)
            || (static_cast<int32>(used_blocks.size()) <= selected_block_number)
            || (static_cast<int32>(block_anim.size()) <= selected_block_number)) return;

        block_number = selected_block_number;
        Block* selected_block = used_blocks.at(block_number);
        const Point selected_block_pos = { selected_block->GetPos().first, selected_block->GetPos().second };
        const Point first_piece_pos = selected_block_pos + Point{ selected_block->GetPiece(0, 0).x, selected_block->GetPiece(0, 0).y };
        original_put_at = {
            (first_piece_pos.x - offset.x - cell_size / 2) / cell_size,
            (first_piece_pos.y - offset.y - cell_size / 2) / cell_size
        };
        if ((original_put_at.x < 0) || (board_width <= original_put_at.x)
            || (original_put_at.y < 0) || (board_height <= original_put_at.y)) {
            original_put_at = { -1,-1 };
            return;
        }
        for (int y = 0; y < selected_block->Size().second; y++) {
            for (int x = 0; x < selected_block->Size().first; x++) {
                if (selected_block->GetPiece(x, y).content == '$') continue;
                const int32 board_x = original_put_at.x + x;
                const int32 board_y = original_put_at.y + y;
                if ((board_x < 0) || (board_width <= board_x)
                    || (board_y < 0) || (board_height <= board_y)
                    || (board_usage[board_y][board_x] != num)) {
                    original_put_at = { -1,-1 };
                    return;
                }
            }
        }

        for (int y = 0;y < board_height;y++) {
            for (int x = 0;x < board_width;x++) {
                if (board_usage[y][x] == num) {//同じブロックのマスなら
                    //防御、攻撃マスが含まれているときの処理
                    if (board_number[y][x] == 16777217) {//攻
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < board_width; i++) {
                                if (board_number[y][i] == 16777217)return false;
                            }return true;
                            }())board_off_def[y] = 0;
                    } else if (board_number[y][x] == 16777218) {//防
                        board_number[y][x] = 0;
                        if ([&]()->bool {
                            for (int i = 0; i < board_width; i++) {
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

        block_rotation_count = 0;
        was_block_on_board = true;
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
        block_hand_pos.push_back(hand_pos);//手札の位置を記録
        block_anim.push_back(3);
        block_number = static_cast<int>(used_blocks.size()) - 1; //ブロックの番号を更新
    } else {//既出のブロックなら
        block_number = static_cast<int>(distance(used_blocks.begin(), itr)); //ブロックの番号を更新
        block_anim[block_number] = 3;
    }
    original_put_at = { -1,-1 };
    block_rotation_count = 0;
    was_block_on_board = false;
    is_block_selected = true;
    is_board_active = true; // Boardをアクティブにする
}
