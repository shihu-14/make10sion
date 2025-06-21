#include "Board.hpp"
using namespace std;

Board::Board() : 
	board_usage(Size{ 7,6 }, -1), 
	board_number(Size{ 7,6 }, 0),
	board_effect(Size{ 7,6 }, 0),
	board_coordinate(Size{ 7,6 },Point{ 0,0 })
{
	
}

void Board::Update(int32 idx){//idx : 0:バトル中, 1:リザルト(マス解放時)
	//ここでBoardのメソッドを呼び出すので、
	//Battle.cppではこの関数を毎フレーム呼び出してください.
}