#include "../src/Board.hpp"  
#include <Siv3D.hpp>

void Board::CalcRow() {

	//数字の集計
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 7; j++) {
			if (board_usage[i][j] != 1) continue; // 使用されていないブロックはスキップ
			if (board_number[i][j] == 0)continue;
			if ((board_number[i][j] & (1 << 8)) == 0 && (board_number[i][j] & (1 << 16)) == 0 && (board_number[i][j] & (1 << 24)) == 0) {//数字であるか
				board_number[i][j] += board_effect[i][j]; // 効果を加える
				num_on_board.push_back(board_number[i][j]); // 数字部分を取り出す
			}
		}
	}
	std::sort(num_on_board.begin(), num_on_board.end());

	//構文解析するためにボード上の文字列を圧縮 
	for (int i = 0; i < 6; i++) {

		//初期化
		bool before_was_number = false; // 前に見た記号が数字かどうか判断する  
		bool before_was_operator = false; // 前に見た記号が演算子かどうか判断する  
		String function;

		for (int j = 0; j < 7; j++) {

			if (board_usage[i][j] != 1) continue;// 使用されていないブロックはスキップ
			if (board_number[i][j] == 0) continue;// 数字が0のブロックはスキップ

			if (board_number[i][j] & (1 << 8)) { // 演算子のビットが立っているかどうか
				if (before_was_operator == true) continue;
				if (before_was_number == false) continue;
				before_was_operator = true;
				before_was_number = false;

				if (board_number[i][j] & (1 << 0)) { // 足し算  
					function << U'+';
				}
				else if (board_number[i][j] & (1 << 1)) { // 引き算  
					function << U'-';
				}
				else if (board_number[i][j] & (1 << 2)) { // 掛け算  
					function << U'*';
				}
				else if (board_number[i][j] & (1 << 3)) { // 割り算  
					function << U'/';
				}
				continue;
			}

			else if (board_number[i][j] & (1 << 16)) { // Max, Min, Aveのとき  
				if (before_was_number == true) continue;
				if (board_number[i][j] & (1 << 0)) { // Max  
					function += Format(num_on_board.back()); // 修正: int を String に変換  
				}
				else if (board_number[i][j] & (1 << 1)) { // Min  
					function += Format(num_on_board.front()); // 修正: int を String に変換  
				}
				else if (board_number[i][j] & (1 << 2)) { // Ave  
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
				if ((board_number[i][j] & (1 << 0))) {//攻
					board_off_def[i] = 1;
				}
				else if (board_number[i][j] & (1 << 1)) {//守
					board_off_def[i] = 0;
				}
			}

			else {
				if (before_was_number == true) continue; // 前が数字ならスキップ
				before_was_number = true;
				before_was_operator = false;
				function += Format(board_number[i][j]);
			}

		}
		//最後に演算子があったら削除
		if (function.length() > 0 && (function.back() == '+' || function.back() == '-' || function.back() == '*' || function.back() == '/')) {
			function.pop_back();
		}
		Print << function;
		Print << Eval(function);

	}
}

std::pair<int, int> Board::Confirm() {  
    SetStat();
    
	// 数字の集計が終わったので、num_on_boardをクリア

}  

void Board::SetStat() {  
    if (is_board_active == true) {  
        is_board_active = false;  
    } else {  
        is_board_active = true;  
    }  
}


