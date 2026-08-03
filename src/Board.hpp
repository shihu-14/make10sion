#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <vector>
#include <array>
#include <Siv3D.hpp>
#include "Block.hpp"
#include "leric.hpp"

class Board {
private:
	struct BoardBlockState {
		Block* block = nullptr;
		int32 deck_index = -1;
		Point hand_pos = { -1,-1 };
		Point board_anchor = { -1,-1 };
		int32 rotation = 0;
		int32 animation = -1;
	};

	struct BoardBlockSnapshot {
		Block* block = nullptr;
		int32 deck_index = -1;
		Point screen_pos = { -1,-1 };
		Point board_anchor = { -1,-1 };
		int32 rotation = 0;
		int32 stat = 0;
		int32 animation = -1;
	};

	struct DragContext {
		bool active = false;
		int32 board_block_index = -1;
		int32 deck_index = -1;
		Block* block = nullptr;
		bool from_board = false;
		int32 start_stat = 0;
		Point start_screen_pos = { -1,-1 };
		Point hand_pos = { -1,-1 };
		Point board_anchor = { -1,-1 };
		int32 start_rotation = 0;
		int32 rotation_count = 0;
		Array<BoardBlockSnapshot> board_block_snapshots;
	};

	enum class DropType {
		Invalid,
		Place,
		HandBoardSwap,
		BoardBoardSwap,
	};

	struct DropPlan {
		DropType type = DropType::Invalid;
		Point anchor = { -1,-1 };
		int32 target_block_index = -1;
	};

	//variables
	Grid<int32> board_usage = { {-1,-1,-1,-1,-1,-1,-1},
							 {-1,-1,-2,-2,-2,-1,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-1,-2,-2,-2,-1,-1},
							 {-1,-1,-1,-1,-1,-1,-1} };
	Grid<int32> board_number;
	Grid<int32> board_effect_back;
	Grid<int32> board_effect_front;
	Grid<char> board_content;
	Grid<Point> board_coordinate;
	Array<int32> num_on_board;
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };
	Array<double> board_multiply_effect = { 0,0,0,0,0,0 };
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//攻1守0
	Array<int32> result_of_calc = { 0,0,0,0,0,0 };
	const Point offset = { 600,170 };//Boardの左上の絶対座標(バトル時)
	//const Point offset_u = {0,0};//Boardの左上の絶対座標(アンロック時)(使わないかも)
	const double img_scale = 1.8;
	const int32 cell_size = int(50 * img_scale);
	const Texture board_img{ U"../../image/banmen_kuuhaku.png" };
	const Texture chosed_board_img{ U"../../image/special_n.png" };
	const Texture chosable_board_img{ U"../../image/tile_kokodayo.png" };
	const Texture board_frame_img{ U"../../image/tile_flame.png" };
	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };
	Array<BoardBlockState> board_blocks;//盤面上では「このインデックス+1」を番号とする. ターン毎に初期化
	DragContext drag_context;
	int32 add_damage = 0;
	int32 add_armor = 0;
	std::vector<int32> relics_old;
	bool do_armor_raise = false;
	int32 add_damage_by_cards = 0;
	int32 off_count = 3;
	bool is_board_active = false;

	//function
	int32 FindBoardBlockIndex(int32 deck_index) const;
	int32 ResolveDragBlockIndex() const;
	bool IsBoardBlockIndexValid(int32 index) const;
	bool IsDragContextValid() const;
	Point GetBoardCellCenter(Point cell) const;
	Point GetScaledPieceOffset(const Piece& piece) const;
	Point GetBoardBlockScreenPosition(const Block& block, Point anchor) const;
	Point ScreenToBoardCell(Point screen_pos) const;
	Point GetBoardAnchorFromScreenPosition(const Block& block, Point screen_pos) const;
	bool GetBlockCells(const Block& block, Point anchor, Array<Point>& cells) const;
	bool IsBoardBlockPlaced(int32 index) const;
	bool CanPlaceBlock(int32 index, Point anchor, int32 ignored_index_1, int32 ignored_index_2 = -1) const;
	bool BlocksOverlap(int32 index_1, Point anchor_1, int32 index_2, Point anchor_2) const;
	void CaptureBoardBlockSnapshots();
	bool ValidateBoardState(int32 allowed_target_index = -1) const;
	void AssertBoardState(int32 allowed_target_index = -1) const;
	Point PutBlockAt() const;
	DropPlan AnalyzeDrop(Point candidate_anchor) const;
	void PutBlock();
	void ClearBoardBlock(int32 index);
	void UpdateBoardNum(int32 index, Point putAt);
	void SetBoardBlockPosition(int32 index, Point anchor);
	void SetBlockRotation(int32 index, int32 rotation);
	bool RestoreDrag();
	void ClearDrag();
	void GetPieceNum(char content, int y, int x);
	void InitBoardCoordinate();
	void TakeOutBlock(Point pos);
	void AddUsablePlace();
	void RebuildBoardDerivedState();
	void CalcRow();
	void DrawOnlyBoard() const;
	void DrawBlock(Block block_on_board);
	void BlockAnimation(int32 index, Point end_pos);
	void DrawAddPlaceBoard() const;
	void DoRelic(std::vector<int32> relics);

	double inline CalcDist(Point a, Point b) const { return pow((a.x - b.x), 2) + pow((a.y - b.y), 2); };


public:

	Board() :board_number(Size{ 7,6 }, 0),
		board_effect_back(Size{ 7,6 }, 0),
		board_effect_front(Size{ 7,6 }, 0),
		board_content(Size{ 7,6 }, '\0'),
		board_coordinate(Size{ 7,6 }, Point{ 0,0 }),
		relics_old(19, 0)
		{};

	//variables
	int32 unlocked_num = 6;

	//functions
	void InitAll();
	void Discard();
	void Update(int32 idx, std::vector<int32> relics);
	void DrawBoard(int32 idx) const;
	std::pair<int32, int32> Confirm();
	bool PassBlock(Block& selectedBlock, int32 deck_index, const Point hand_pos);
	bool IsBusy() const;
	bool IsDraggingDeck(int32 deck_index) const;
};

#endif
