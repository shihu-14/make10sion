#include "Block.hpp"
#include "Board.hpp"
#include <Siv3D.hpp>

void Board::RebuildBoardDerivedState() {
	board_multiply_effect.fill(0);
	board_off_def.fill(0);
	const int32 board_width = Min(static_cast<int32>(board_usage.width()), static_cast<int32>(board_content.width()));
	const int32 grid_height = Min(static_cast<int32>(board_usage.height()), static_cast<int32>(board_content.height()));
	const int32 state_height = Min(static_cast<int32>(board_multiply_effect.size()), static_cast<int32>(board_off_def.size()));
	const int32 board_height = Min(grid_height, state_height);
	const int32 attack_row_count = Min(Max(off_count, 0), board_height);
	for (int y = 0; y < attack_row_count; y++) {
		board_off_def[y] = 1;
	}

	for (int y = 0; y < board_height; y++) {
		for (int x = 0; x < board_width; x++) {
			if (board_usage[y][x] <= 0) continue;
			const char content = board_content[y][x];
			if (content == 'a') {
				board_multiply_effect[y] = 1.0;
			} else if (content == 'b') {
				board_multiply_effect[y] = 1.5;
			} else if (content == 'c') {
				board_multiply_effect[y] = 2.0;
			} else if (content == 'f') {
				board_off_def[y] = 0;
			} else if (content == 'i') {
				board_off_def[y] = 1;
			}
		}
	}
}

void Board::CalcRow() {
	RebuildBoardDerivedState();
	num_on_board.clear();
	result_of_calc.fill(0);
	const int32 board_width = static_cast<int32>(board_usage.width());
	const int32 board_height = static_cast<int32>(board_usage.height());

	//数字の集計
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			if (board_usage[i][j] <= 0) continue; // 使用されていないブロックはスキップ
			if (board_number[i][j] == 0)continue;
			if ((board_number[i][j] & (1 << 8)) == 0 && (board_number[i][j] & (1 << 16)) == 0 && (board_number[i][j] & (1 << 24)) == 0) {//数字であるか
				num_on_board.push_back(board_number[i][j] + board_effect_front[i][j]); // 数字部分を取り出す
			}

		}
	}
	std::sort(num_on_board.begin(), num_on_board.end());

	//構文解析するためにボード上の文字列を圧縮 
	for (int i = 0; i < board_height; i++) {

		//初期化
		bool before_was_number = false; // 前に見た記号が数字かどうか判断する  
		bool before_was_operator = false; // 前に見た記号が演算子かどうか判断する  
		String function;

		for (int j = 0; j < board_width; j++) {
			//FIXME: board_usage[i][j] != 1 でスキップしているが、これが正しいか確認する必要がある
			if (board_usage[i][j] <= 0) continue;// 使用されていないブロックはスキップ
			if (board_number[i][j] == 0) continue;// 数字が0のブロックはスキップ

			if (board_number[i][j] & (1 << 8)) { // 演算子のビットが立っているかどうか
				if (before_was_operator || !before_was_number) continue;
				before_was_operator = true;
				before_was_number = false;

				if (board_number[i][j] & (1 << 0)) { // 足し算  
					function << U'+';
				} else if (board_number[i][j] & (1 << 1)) { // 引き算  
					function << U'-';
				} else if (board_number[i][j] & (1 << 2)) { // 掛け算  
					function << U'*';
				} else if (board_number[i][j] & (1 << 3)) { // 割り算  
					function << U'/';
				}
				continue;
			}

			else if (board_number[i][j] & (1 << 16)) { // Max, Min, Aveのとき  
				if (before_was_number == true || num_on_board.isEmpty()) continue;
				before_was_number = true;
				if (board_number[i][j] & (1 << 0)) { // Max  
					function += Format(num_on_board.back()); // 修正: int を String に変換  
				} else if (board_number[i][j] & (1 << 1)) { // Min
					function += Format(num_on_board.front()); // 修正: int を String に変換
				} else if (board_number[i][j] & (1 << 2)) { // Ave  
					double ave = 0;
					for (auto& num : num_on_board) {
						ave += num;
					}
					ave /= num_on_board.size();
					function += Format(ave); // 修正: double を String に変換
				}
				continue;
			}

			else if (board_number[i][j] & (1 << 24)) { // null扱いのブロックのビットが立っているかどうか
				continue;
			}

			else {
				if (before_was_number == true) continue; // 前が数字ならスキップ
				before_was_number = true;
				before_was_operator = false;
				function += Format(board_number[i][j] + board_effect_front[i][j]);
			}

		}
		//最後に演算子があったら削除
		if (function.length() > 0 && (function.back() == '+' || function.back() == '-' || function.back() == '*' || function.back() == '/')) {
			function.pop_back();
		}
		if (function.size() != 0)result_of_calc[i] = Eval(function);
	}
}



std::pair<int, int> Board::Confirm() {
	is_board_active = !is_board_active;
	CalcRow();
	int attack = 0, defense = 0;
	for (int i = 0; i < 6; i++) {
		if (board_off_def[i] == 1) { //攻撃側の行  
			attack += result_of_calc[i] * (board_multiply[i] + board_multiply_effect[i]);
		} else if (board_off_def[i] == 0) { //防御側の行  
			defense += result_of_calc[i] * (board_multiply[i] + board_multiply_effect[i]);
		}
	}
	int32 placed_block_count = 0;
	for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
		if (IsBoardBlockPlaced(i)) placed_block_count++;
	}
	attack += add_damage_by_cards * placed_block_count;
	if (do_armor_raise) {
		if (defense < 6)defense = 6;
	}
	return { attack, defense };
}


void Board::Discard() {
	board_number.fill(0);
	board_content.fill('\0');
	num_on_board.clear();
	result_of_calc.fill(0);
	for (auto& state : board_blocks) {
		if (state.board_anchor != Point{ -1,-1 }) state.animation = 2;
	}
	board_effect_front = board_effect_back;
	board_effect_back.fill(0);
	RebuildBoardDerivedState();
}



void Board::AddUsablePlace(Point cursor_pos) {
	const int32 board_width = static_cast<int32>(board_usage.width());
	const int32 board_height = static_cast<int32>(board_usage.height());
	const Point cell = ScreenToBoardCell(cursor_pos);
	if (cell == Point{ -1,-1 }) return;
	const int32 bx = cell.x;
	const int32 by = cell.y;
	if (board_usage[by][bx] != -2)return;

	board_usage[by][bx] = 0;
	for (int i = 0; i < board_height; i++) {//使用不可の所の初期化
		for (int j = 0; j < board_width; j++) {
			if (board_usage[i][j] != 0)board_usage[i][j] = -1;
		}
	}
	//ここ以降で使用可能に隣接する使用不可の所の計算を行う
	for (int i = 0; i < board_height; i++) {
		for (int j = 0; j < board_width; j++) {
			if (board_usage[i][j] != -1)continue;
			if (i < 2) {
				if ((i + 1 < board_height) && (board_usage[i + 1][j] == 0)) board_usage[i][j] = -2;
			} else if (i < 4) {
				if ((j + 1 < board_width) && (board_usage[i][j + 1] == 0)) board_usage[i][j] = -2;
				if ((0 < j) && (board_usage[i][j - 1] == 0)) board_usage[i][j] = -2;
			} else {
				if ((0 < i) && (board_usage[i - 1][j] == 0)) board_usage[i][j] = -2;
			}
		}
	}
	//ココまで

}





bool Board::UpdateBoardNum(int32 index, Point putAt) {
	if (!IsBoardBlockIndexValid(index)) return false;
	Block* block = board_blocks[index].block;
	Array<Point> cells;
	if (!GetBlockCells(*block, putAt, cells)) return false;
	for (int i = 0; i < block->Size().second; i++) {
		for (int j = 0; j < block->Size().first; j++) {
			char content = block->GetPiece(j, i).content;
			if (content == '$')continue; // $は無視
			board_usage[putAt.y + i][putAt.x + j] = board_blocks[index].deck_index + 1;
			GetPieceNum(content, putAt.y + i, putAt.x + j); // 数字の取得&マスの変更
		}
	}
	return true;
}



void Board::GetPieceNum(char content, int y, int x) {
	board_number[y][x] = 0;
	board_effect_back[y][x] = 0;
	board_content[y][x] = content;
	if (content == '+') {
		board_number[y][x] = 257; return; // 足し算
	} else if (content == '-') {
		board_number[y][x] = 258; return; // 引き算
	} else if (content == '*') {
		board_number[y][x] = 260; return; // 掛け算
	} else if (content == '/') {
		board_number[y][x] = 264; return; // 割り算
	} else if (content == 'a') {
		return;
	} else if (content == 'b') {
		return;
	} else if (content == 'c') {
		return;//未定
	} else if (content == 'd') {
		return;//未定
	} else if (content == 'e') {
		board_number[y][x] = 65540; return;//ave
	} else if (content == 'f') {
		board_number[y][x] = 16777218; return;//守
	} else if (content == 'g') {
		board_number[y][x] = 65537; return;//max
	} else if (content == 'h') {
		board_number[y][x] = 65538; return;//min
	} else if (content == 'i') {
		board_number[y][x] = 16777217; return;//攻
	} else if (content == 'j') {
		board_number[y][x] = 1;
		board_effect_back[y][x] = 2;
		return;
	} else if (content == 'k') {
		board_number[y][x] = 2;
		board_effect_back[y][x] = 2;
		return;
	} else if (content == 'l') {
		board_number[y][x] = 2;
		board_effect_back[y][x] = 4;
		return;
	} else if (content == 'm') {//数字の12
		board_number[y][x] = 12;
		return;
	} else if (content == 'n') {
		//何もしない
	} else if (content == 'o') {
		board_number[y][x] = 2;
		board_effect_back[y][x] = 1;
		return;
	} else if (content == 'p') {
		board_number[y][x] = 3;
		board_effect_back[y][x] = 1;
		return;
	} else board_number[y][x] = content - '0';
	return;
}
