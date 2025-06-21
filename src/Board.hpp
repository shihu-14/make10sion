#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <vector>
#include <array>
#include <Siv3D.hpp>
#include "Block.hpp"

class Board{
private://Write private functions or varables here.

	//variables
	Grid<int32> board_usage;
	Grid<int32> board_number;
	Grid<int32> board_effect;
	Grid<Point> board_coordinate;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<int32> num_on_board;
	Array<int32> result_of_calc= { 0,0,0,0,0,0 };
	bool is_block_selected = false;
	int32 blockNum;
	Block block;
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//攻1守0
	const Texture board_img{U"../image/banmen_kuuhaku.png"};
	const std::array<Texture, 4> block_sides = {Texture{U"../image/card_sukima_top.png"},
												Texture{U"../image/card_sukima_bottom.png"},
												Texture{U"../image/card_sukima_right.png"},
												Texture{U"../image/card_sukima_left.png"}};
	const Point offset = {0,0};//Boardの左上の絶対座標
	const int32 cell_size = 50;

	//function
	Point PutBlockAt();
	void PutBlock();
	Array<std::pair<int32,int32>> TakeOutBlock();//{x, y}で返す
	void AddUsablePlace();
	void ResetBoard();
	void CalcRow();
	void UpdateBoardNum(Point putAt);
	void InitBoardCoordinate();
	void GetPieceNum(char content, int y, int x);

	
	
public://Write public functions here.

	Board();
	//variables
	bool is_board_active = false;
	int32 unlocked_num = 6;

	//functions
	void Update(int32 idx);
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock(const Block& selectedBlock, const std::vector<Block> deck);
	void DrawBoard();
};

#endif
