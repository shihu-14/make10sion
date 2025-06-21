#include "../src/Board.hpp"  
#include <Siv3D.hpp>  

std::pair<int, int> Board::Confirm() {  
    SetStat();  
    bool before_was_number = false; // 前に見た記号が数字かどうか判断する  
    bool before_was_operator = false; // 前に見た記号が演算子かどうか判断する  
    String function;  
    for (int i = 0; i < 6; i++) {  
        for (int j = 0; j < 7; j++) {  
            if (board_usage[j][i] != 1) continue;  
            if (board_number[j][i] == 0) continue;  
            if ((board_number[j][i] & (1 << 8))) { // 演算子のビットが立っているかどうか  
                if (before_was_operator == true) continue;  
                if (before_was_number == false) continue;  
                before_was_operator = true;  
                before_was_number = false;  
                if (board_number[j][i] & (1 << 0)) { // 足し算  
                    function << '+';  
                } else if (board_number[j][i] & (1 << 1)) { // 引き算  
                    function << '-';  
                } else if (board_number[j][i] & (1 << 2)) { // 掛け算  
                    function << '*';  
                } else if (board_number[j][i] & (1 << 3)) { // 割り算  
                    function << '/';  
                }  
                continue;  
            }  
            if (board_number[j][i] & (1 << 16)) { // Max, Min, Aveのとき  
                if (before_was_number == true) continue;  
                sort(num_on_board.begin(), num_on_board.end());  
                if (board_number[j][i] & (1 << 0)) { // Max  
                    function << Format(num_on_board.back()); // 修正: int を String に変換  
                }else if(board_number[j][i] & (1 << 1)) { // Min  
					function << Format(num_on_board.front()); // 修正: int を String に変換  
				}
				else if (board_number[j][i] & (1 << 2)) { // Ave  
					double ave = 0;
					for (auto& num : num_on_board) {
						ave += num;
					}
					ave /= num_on_board.size();
					function << Format(ave); // 修正: double を String に変換
				}
				continue;
            }
			if (board_number[j][i] & (1 << 24)) { // 数字のビットが立っているかどうか
				if ((board_number[j][i] & )1 == 0) {

				}
			}
        }  
    }  
}  

void Board::SetStat() {  
    if (is_board_active == true) {  
        is_board_active = false;  
    } else {  
        is_board_active = true;  
    }  
}
