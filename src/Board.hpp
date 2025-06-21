#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <Siv3D.hpp>
#include "Block.hpp"

class Board{
private://Write private functions or varables here.

	//variables
	Grid<int32> board_usage;
	Grid<char> board_number;
	Grid<int32> board_effect;
	Grid<Point> board_coordinate;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<int> num_on_board;
	Block block;
	Array<int> board_off_def = { 1,1,1,0,0,0 };//çU1éÁ0

	//function
	void PutBlock();
	Array<std::pair<int,int>> TakeOutBlock();
	void AddUsablePlace();
	void ResetBoard();


	
	
public://Write public functions here.

	Board();
	//variables
	bool is_board_active = false;

	//functions
	void Update(int idx);
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock(const Block& selectedBlock);
	void DrawBoard();
};

#endif
