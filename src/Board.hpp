#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <vector>
#include <array>
#include <Siv3D.hpp>
#include "Block.hpp"
#include "leric.hpp"

class Board{
private:

	//variables
	Grid<int32> board_usage={{-1,-1,-1,-1,-1,-1,-1},
							 {-1,-1,-2,-2,-2,-1,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-2,-2,-2,-2,-1,-1},
							 {-1,-1,-1,-1,-1,-1,-1}};
	Grid<int32> board_number;
	Grid<int32> board_effect_back;
	Grid<int32> board_effect_front;
	Grid<Point> board_coordinate;
	Array<int32> num_on_board;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<double> board_multiply_effect = { 0,0,0,0,0,0 };
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//攻1守0
	Array<int32> result_of_calc= { 0,0,0,0,0,0 };
	bool is_block_selected = false;
	int32 blockNum;
	Block block;
	const Point offset = {600,100};//Boardの左上の絶対座標(バトル時)
	const Point offset_u = {0,0};//Boardの左上の絶対座標(アンロック時)(使わないかも)
	const double img_scale = 1.8;
	const int32 cell_size = 50 * img_scale;
	const Texture board_img{U"../../image/banmen_kuuhaku.png"};
	const Texture chosed_board_img{ U"../../image/special_n.png" };
	const Texture chosable_board_img{ U"../../image/tile_kokodayo.png" };
	const Texture board_frame_img{ U"../../image/tile_flame.png" };
	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };
	Array<Block> used_blocks;//盤面に出てきたブロックの配列. blockNumは「このインデックス+1」とする
	Array<Point> block_hand_pos;//各ブロックの手札上の位置を保存
	Array<int32> block_anim;//実質描画順	-1:盤面上に無い, 0:ボード上, 1:手札へ, 2:捨札へ, 3:アニメーション無し
	int32 add_damage = 0;
	int32 add_armor = 0;
	std::vector<int32> relics_old;
	bool do_armor_raise = false;
	int32 add_damage_by_cards = 0;
	int32 off_count = 3;

	//function
	Point PutBlockAt();
	void PutBlock();
	void UpdateBoardNum(Point putAt);
	void GetPieceNum(char content, int y, int x);
	void InitBoardCoordinate();
	void TakeOutBlock(Point pos);
	void AddUsablePlace();
	void CalcRow();
	void DrawOnlyBoard() const;
	void DrawBlock(Block block_on_board);
	void BlockAnimation(Block moving_block, Point end_pos, int32 anim_num);
	void DrawAddPlaceBoard() const;
	void DoRelic(std::vector<int32> relics);

	double CalcDist(Point a, Point b);
	
	
public:

	Board();

	//variables
	bool is_board_active = false;
	int32 unlocked_num = 6;
	int32 num_of_used_card;

	//functions
	void InitAll();
	void Discard();
	void Update(int32 idx, std::vector<int32> relics);
	void DrawBoard(int32 idx) const;
	void SetStat();
	std::pair<int32, int32> Confirm();
	void PassBlock(const Block& selectedBlock, const Point hand_pos);
};

#endif
