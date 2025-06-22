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
	Grid<int32> board_effect_back;
	Grid<int32> board_effect_front;
	Grid<Point> board_coordinate;
	Array<int32> num_on_board;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//攻1守0
	Array<int32> result_of_calc= { 0,0,0,0,0,0 };
	bool is_block_selected = false;
	int32 blockNum;
	Block block;
	const Point offset = {0,0};//Boardの左上の絶対座標(バトル時)
	const Point offset_u = {0,0};//Boardの左上の絶対座標(アンロック時)(使わないかも)
	const int32 cell_size = 50;
	const Texture board_img{U"../image/banmen_kuuhaku.png"};
	const Texture chosed_board_img{ U"../image/special_n.png" };
	const Texture chosable_board_img{ U"../image/tile_kokodayo.png" };
	Array<Block> used_blocks;//盤面に出てきたブロックの配列. blockNumは「このインデックス+1」とする
	Array<Point> block_hand_pos;//各ブロックの手札上の位置を保存
	Array<int32> block_anim;//実質描画順	-1:盤面上に無い, 0:ボード上, 1:手札へ, 2:捨札へ, 3:アニメーション無し

	//function
	Point PutBlockAt();
	void PutBlock();
	void UpdateBoardNum(Point putAt);
	void GetPieceNum(char content, int y, int x);
	void InitBoardCoordinate();
	void TakeOutBlock(Point pos);
	void AddUsablePlace();
	void ResetBoard();
	void CalcRow();
	void DrawBlock(Block block_on_board);
	void BlockAnimation(Block moving_block, Point end_pos);
	void DrawAddPlaceBoard();

	double CalcDist(Point a, Point b);
	
	
public:

	Board();

	//variables
	bool is_board_active = false;
	int32 unlocked_num = 6;
	int32 num_of_used_card;

	//functions
	void Update(int32 idx);
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock(const Block& selectedBlock, const Point hand_pos);
	void DrawBoard();
};

#endif
