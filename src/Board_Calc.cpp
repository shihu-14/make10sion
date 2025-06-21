#include "../src/Board.hpp"  
#include "Block.hpp"
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
				if ((board_number[i][j] & (1 << 0))) {//攻なら
					board_off_def[i] = 1;//ラインを攻に設定
				}
				else if (board_number[i][j] & (1 << 1)) {//守
					board_off_def[i] = 0;//ラインを守に設定
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
		 result_of_calc[i] = Eval(function);

	}
}



std::pair<int, int> Board::Confirm() {  
    SetStat();
	CalcRow();
	int attack, defense;
	for (int i = 0; i < 6; i++) {
		if (board_off_def[i] == 1) { //攻撃側の行  
			attack = result_of_calc[i];  
		} else if (board_off_def[i] == 0) { //防御側の行  
			defense = result_of_calc[i];  
		}
	}
	return { attack, defense };
}  



void Board::SetStat() {//ボードの操作状態を設定する
	if (is_board_active == true) {
		is_board_active = false;
	}
	else {
		is_board_active = true;
	}
}



void Board::ResetBoard() {
	board_number.fill(0);
	board_effect.fill(0);
	num_on_board.clear();
	result_of_calc.fill(0);
	board_off_def = { 1,1,1,0,0,0 }; // 初期化: 攻撃側の行を1に設定
}



void Board::AddUsablePlace(){
	
}



void Board::UpdateBoardNum(Point putAt){
	for (int i = 0; i < block.Size().second; i++) {
		for (int j = 0; j < block.Size().first; j++) {
			char content = block.GetPiece(j, i).content;
			if (content == '$')continue; // $は無視
			board_usage[putAt.y + i][putAt.x + j] = blockNum;
			GetPieceNum(content, putAt.y + i, putAt.x + j); // 数字の取得&マスの変更
		}
	}
}



void Board::GetPieceNum(char content, int y, int x) {
	if (content == '+') {
		board_number[y][x] = 257; return; // 足し算
	}else if (content == '-') {
		board_number[y][x] = 258; return; // 引き算
	}else if (content == '*') {
		board_number[y][x] = 260; return; // 掛け算
	}else if (content == '/') {
		board_number[y][x] = 264; return; // 割り算
	}else if (content == 'a') {
		return;
	}else if (content == 'b') {
		return;
	}else if (content == 'c') {
		return;
	}else if (content == 'd') {
		return;
	}else if (content == 'e') {
		board_number[y][x] = 65540; return;//ave
	}else if (content == 'f') {
		board_number[y][x] = 16777218; return;//守
	}else if (content == 'g') {
		board_number[y][x] = 65537; return;//max
	}else if (content == 'h') {
		board_number[y][x] = 65538; return;//min
	}else if (content == 'i') {
		board_number[y][x] = 16777217; return;//攻
	}else if (content == 'j') {
		board_number[y][x] = 1;
		board_effect[y][x] = 2;
		return;
	}else if (content == 'k') {
		board_number[y][x] = 2;
		board_effect[y][x] = 2;
		return;
	}else if (content == 'l') {
		board_number[y][x] = 2;
		board_effect[y][x] = 4;
		return;
	}else if (content == 'm') {//数字の12
		board_number[y][x] = 12;
		return;
	}else if (content == 'n') {
		//何もしない
	}else if (content == 'o') {
		board_number[y][x] = 2;
		board_effect[y][x] = 1;
		return;
	}else if (content == 'p') {
		board_number[y][x] = 3;
		board_effect[y][x] = 1;
		return;
	}
	else board_number[y][x] = content - '0';
}




