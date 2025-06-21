#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <vector>
#include <array>
#include <Siv3D.hpp>
#include "Block.hpp"

class Board{
private:

	//variables
	Grid<int32> board_usage;
	Grid<int32> board_number;
	Grid<int32> board_effect;
	Grid<Point> board_coordinate;
	Array<int32> num_on_board;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//攻1守0
	Array<int32> result_of_calc= { 0,0,0,0,0,0 };
	bool is_block_selected = false;
	int32 blockNum;
	Block block;
	const Point offset = {0,0};//Boardの左上の絶対座標
	const int32 cell_size = 50;
	const Texture board_img{U"../image/banmen_kuuhaku.png"};
	std::map<Block, Point> block_hand_pos;


	//function
	Point PutBlockAt();
	void PutBlock();
	void UpdateBoardNum(Point putAt);
	void GetPieceNum(char content, int y, int x);
	void InitBoardCoordinate();
	Array<std::pair<int32,int32>> TakeOutBlock();//{x, y}で返す
	void AddUsablePlace();
	void ResetBoard();
	void CalcRow();
	void DrawBlockOnBoard(Block block_on_board);
	void BlockAnimation(Block moving_block, Point end_pos);

	
	
public:

	Board();

	//variables
	bool is_board_active = false;
	int32 unlocked_num = 6;

	//functions
	void Update(int32 idx);
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock(const Block& selectedBlock, const Point hand_pos, const std::vector<Block> deck);
	void DrawBoard();
};

#endif
