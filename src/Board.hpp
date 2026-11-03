#ifndef Board_HPP
#define Board_HPP

#include <utility>
#include <vector>
#include <array>
#include <Siv3D.hpp>
#include "Block.hpp"
#include "BoardCalculationRules.hpp"
#include "BattleCardRules.hpp"
#include "BattleLayoutRules.hpp"
#include "GameStateRules.hpp"
#include "leric.hpp"

// バトルシーンから渡される盤面入力を表す．
struct BoardInputFrame {
	Point cursor = { 0,0 };
	bool left_down = false;
	bool left_pressed = false;
	bool left_up = false;
	bool rotate_pressed = false;
	bool focused = true;
	double delta_seconds = 0.0;
};

class Board {
private:
	// 盤面上にあるカードの状態を保持する．
	struct BoardBlockState {
		Block* block = nullptr;
		int32 deck_index = -1;
		int32 hand_slot = -1;
		Point hand_pos = { -1,-1 };
		Point board_anchor = { -1,-1 };
		int32 rotation = 0;
		BattleCardRules::CardLifecycle lifecycle = BattleCardRules::CardLifecycle::InDeck;
		BattleCardRules::ReturnMotion visual_motion;
		Point motion_start = { 0,0 };
		Point motion_end = { 0,0 };
	};

	// ドラッグ中のカードと開始位置を保持する．
	struct DragContext {
		bool active = false;
		int32 deck_index = -1;
		bool from_board = false;
		Point start_screen_pos = { -1,-1 };
		Point board_anchor = { -1,-1 };
		Point cursor_offset = { 0,0 };
		int32 start_rotation = 0;
		int32 rotation_steps = 0;
	};

	using DropType = BattleCardRules::DropResult;

	struct DropPlan {
		DropType type = DropType::ReturnToHand;
		Point anchor = { -1,-1 };
		int32 target_deck_index = -1;
	};

	// 盤面の状態を-1=未使用，-2=解放可能，0=使用可能で管理する．
	Grid<int32> board_usage = { {-1,-1,-1,-1,-1,-1,-1},
							 {-1,-1,-2,-2,-2,-1,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-2, 0, 0, 0,-2,-1},
							 {-1,-1,-2,-2,-2,-1,-1},
							 {-1,-1,-1,-1,-1,-1,-1} };
	Grid<int32> board_number;
	Grid<int32> board_effect_back;
	Grid<int32> board_effect_front;
	Grid<int32> board_effect_committed;
	Grid<char> board_content;
	Grid<Point> board_coordinate;
	Grid<BoardCalculationRules::ExpressionCellUsage> expression_cell_usage;
	Array<int32> num_on_board;
	const Array<double> board_multiply_base = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };//行ごとの基本倍率を保持する．
	Array<double> board_multiply = { 2.0, 1.5, 1.0, 1.0, 1.5, 2.0 };//現在の行倍率を保持する．
	Array<double> board_multiply_effect = { 0,0,0,0,0,0 };//レリックと記号による倍率補正を保持する．
	Array<int32> board_off_def = { 1,1,1,0,0,0 };//1を攻撃列，0を防御列として扱う．
	Array<int32> result_of_calc = { 0,0,0,0,0,0 };
	Array<bool> row_valid = { true,true,true,true,true,true };
	const Point offset = BattleLayoutRules::BoardOffset;//盤面左上の絶対座標を保持する．
		const double img_scale = 1.8;//盤面画像の表示倍率を表す．
	const int32 cell_size = int(50 * img_scale);//50ピクセルのマスを表示倍率に合わせる．
	const Texture board_img{ U"../../image/banmen_kuuhaku.png" };
	const Texture chosed_board_img{ U"../../image/special_n.png" };
	const Texture chosable_board_img{ U"../../image/tile_kokodayo.png" };
	const Texture board_frame_img{ U"../../image/tile_flame.png" };
	const Texture rotate_icon{ U"../../image/rotate.png" };
	const double rotate_hint_scale = 1.5;//回転ヒント全体の表示倍率を表す．
	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };
	Array<BoardBlockState> board_blocks;//board_usageの正数はdeck_index+1を表す．ターンごとに初期化する．
	DragContext drag_context;
	int32 add_damage = 0;//レリックによる攻撃補正を保持する．
	int32 add_armor = 0;//レリックによる防御補正を保持する．
	bool do_armor_raise = false;//防御力を最低値まで引き上げるか表す．
	int32 add_damage_by_cards = 0;//配置カード数による攻撃補正を保持する．
	bool m_calculationChanged = true;
	int32 off_count = 3;//初期状態で先頭3行を攻撃列として扱う．
	std::vector<GameStateRules::CardZoneChange> pending_zone_changes;

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
	void QueueZoneChange(int32 deck_index, GameStateRules::CardZone expected,
		GameStateRules::CardZone destination);
	Point PutBlockAt(Point screen_pos) const;
	DropPlan AnalyzeDrop(Point candidate_anchor, Point release_cursor, Point release_screen_pos) const;
	void PutBlock(Point release_cursor, Point release_screen_pos);
	void ClearBoardBlock(int32 index);
	bool HasBoardOccupancy(int32 index) const;
	void UpdateBoardNum(int32 index, Point putAt);
	void SetBoardBlockPosition(int32 index, Point anchor);
	void SetBlockRotation(int32 index, int32 rotation);
	void RotateDraggedBlock();
	void SwapBoardBlocks(int32 selected_index, int32 target_index);
	void ReturnDraggedBlockToHand();
	void RestoreDraggedBlockToBoard();
	void RollbackDraggedBlock();
	void ClearDrag();
	void StartVisualReturn(int32 index, BattleCardRules::CardLifecycle lifecycle, Point end_pos);
	void UpdateVisualMotions(double delta_seconds);
	void GetPieceNum(char content, int y, int x);
	void InitBoardCoordinate();
	void TakeOutBlock(Point pos, Point cursor_pos);
	void RebuildBoardDerivedState();
	void CalcRow();
	Grid<double> GetBoardCellAlphas(const BoardBlockState& state) const;
	void DrawOnlyBoard() const;
	void DrawAddPlaceBoard() const;
	void DoRelic(const std::vector<int32>& relics);

	double DistanceSquared(Point a, Point b) const {
		double dx = a.x - b.x;
		double dy = a.y - b.y;
		return dx * dx + dy * dy;
	}

public:

	// 盤面を7列6行で初期化する．
	Board() :board_number(Size{ 7,6 }, 0),
		board_effect_back(Size{ 7,6 }, 0),
		board_effect_front(Size{ 7,6 }, 0),
		board_effect_committed(Size{ 7,6 }, 0),
		board_content(Size{ 7,6 }, '\0'),
		board_coordinate(Size{ 7,6 }, Point{ 0,0 }),
		expression_cell_usage(Size{ 7,6 }, BoardCalculationRules::ExpressionCellUsage::NonExpression)
		{};

	void BeginBattle(const GameStateRules::BoardProgress& progress);
	void BeginTurn();
	void EndTurn();
	void BeginUnlockSelection(const GameStateRules::BoardProgress& progress);
	Point GetBoardCellAt(Point screen_pos) const;
	void Discard();
	void Update(int32 idx, const std::vector<int32>& relics, const BoardInputFrame& input, bool allow_input = true);
	void DrawBoard(int32 idx, bool show_calculation_values = true) const;
	void DrawInteractionOverlay() const;
	std::pair<int32, int32> Confirm() const;
	bool ConsumeCalculationChange();
	bool RegisterHandBlock(Block& block, int32 deck_index, int32 hand_slot, Point hand_pos);
	void DetachCard(int32 deck_index);
	bool PassBlock(Block& selectedBlock, int32 deck_index, Point cursor_pos);
	void CancelActiveDrag();
	void CompleteVisualMotions();
	bool CanStartHandDrag(int32 deck_index) const;
	bool ShouldDrawAsHand(int32 deck_index) const;
	bool IsDragging() const;
	std::vector<GameStateRules::CardZoneChange> ConsumeZoneChanges();
};

#endif
