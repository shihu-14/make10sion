#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <Siv3D.hpp>

class Board{
private://Write private functions or varables here.

	//variables
	Grid<int32> grid_usage(Size{ 7,6 },-1);
	Grid<char> grid_number(Size{7,6}, 0);
	Grid<int32> grid_effect(Size{7,6}, 0);
	Grid<std::pair<int32, int32>> grid_corrdinate(Size{7,6},pair<int32>);
	std::array<int, 6> grid_multiply;
	std::array<int, 42> num_on_grid;

	//function
	void PutBlock();
	void TakeOutBlock();
	void AddUsablePlace();
	void ResetBoard();


	
	
public://Write public functions here.

	//variables
	bool is_board_active = false;

	//functions
	void Update(int idx);
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock();
	void DrawBoard();
};

#endif
