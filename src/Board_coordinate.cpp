#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include "BattleCardRules.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using namespace std;

int32 Board::FindBoardBlockIndex(int32 deck_index) const {
    for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
        if (board_blocks[i].deck_index == deck_index) return i;
    }
    return -1;
}

int32 Board::ResolveDragBlockIndex() const {
	return drag_context.active ? FindBoardBlockIndex(drag_context.deck_index) : -1;
}

bool Board::IsBoardBlockIndexValid(int32 index) const {
    return (0 <= index)
        && (index < static_cast<int32>(board_blocks.size()))
        && (board_blocks[index].block != nullptr)
        && (0 <= board_blocks[index].deck_index);
}

bool Board::IsDragContextValid() const {
	const int32 index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(index)) return false;
	return board_blocks[index].lifecycle == (drag_context.from_board
		? BattleCardRules::CardLifecycle::DraggingFromBoard
		: BattleCardRules::CardLifecycle::DraggingFromHand);
}

Point Board::GetBoardCellCenter(Point cell) const {
	return BattleLayoutRules::BoardCellCenter(cell);
}

Point Board::GetScaledPieceOffset(const Piece& piece) const {
    return {
        static_cast<int32>(std::lround(piece.x * img_scale)),
        static_cast<int32>(std::lround(piece.y * img_scale))
    };
}

Point Board::GetBoardBlockScreenPosition(const Block& block, Point anchor) const {
    if ((block.Size().first <= 0) || (block.Size().second <= 0)) return { -1,-1 };
    return GetBoardCellCenter(anchor) - GetScaledPieceOffset(block.GetPiece(0, 0));
}

Point Board::ScreenToBoardCell(Point screen_pos) const {
	return BattleLayoutRules::BoardCellAt(screen_pos);
}

Point Board::GetBoardAnchorFromScreenPosition(const Block& block, Point screen_pos) const {
    if ((block.Size().first <= 0) || (block.Size().second <= 0)) return { -1,-1 };
    const Point first_piece_pos = screen_pos + GetScaledPieceOffset(block.GetPiece(0, 0));
    const double anchor_x = static_cast<double>(first_piece_pos.x - offset.x - cell_size / 2) / cell_size;
    const double anchor_y = static_cast<double>(first_piece_pos.y - offset.y - cell_size / 2) / cell_size;
    return {
        static_cast<int32>(std::lround(anchor_x)),
        static_cast<int32>(std::lround(anchor_y))
    };
}

bool Board::GetBlockCells(const Block& block, Point anchor, Array<Point>& cells) const {
    cells.clear();
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    for (int32 y = 0; y < block.Size().second; y++) {
        for (int32 x = 0; x < block.Size().first; x++) {
            if (block.GetPiece(x, y).content == '$') continue;
            const Point cell = anchor + Point{ x, y };
            if ((cell.x < 0) || (board_width <= cell.x)
                || (cell.y < 0) || (board_height <= cell.y)) return false;
            cells.push_back(cell);
        }
    }
    return !cells.isEmpty();
}

bool Board::IsBoardBlockPlaced(int32 index) const {
    if (!IsBoardBlockIndexValid(index)) return false;
    const BoardBlockState& state = board_blocks[index];
    if (state.board_anchor == Point{ -1,-1 }) return false;
    Array<Point> cells;
    if (!GetBlockCells(*state.block, state.board_anchor, cells)) return false;
    for (const auto& cell : cells) {
        if (board_usage[cell.y][cell.x] != state.deck_index + 1) return false;
    }
    size_t occupied_count = 0;
    for (const auto& usage : board_usage) {
        if (usage == state.deck_index + 1) occupied_count++;
    }
    return occupied_count == cells.size();
}

Point Board::PutBlockAt(Point screen_pos) const {//blockの置ける場所を確認．blockの(0, 0)のピースのボード座標を返す
    assert(IsDragContextValid());
    const Block& block = *board_blocks[ResolveDragBlockIndex()].block;
    return GetBoardAnchorFromScreenPosition(block, screen_pos);
}

Board::DropPlan Board::AnalyzeDrop(Point candidate_anchor, Point release_cursor, Point release_screen_pos) const {
    DropPlan plan;
    assert(IsDragContextValid());
    const int32 selected_index = ResolveDragBlockIndex();
    if (candidate_anchor == Point{ -1,-1 }) return plan;
    const Block& selected_block = *board_blocks[selected_index].block;
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());

    BattleCardRules::BoardSnapshot snapshot;
    snapshot.width = board_width;
    snapshot.height = board_height;
    snapshot.cells.reserve(static_cast<size_t>(board_width * board_height));
    for (int32 y = 0; y < board_height; y++) {
        for (int32 x = 0; x < board_width; x++) {
            const int32 usage = board_usage[y][x];
            snapshot.cells.push_back({ usage >= 0, usage > 0 ? usage - 1 : BattleCardRules::EmptyCardId });
        }
    }

    BattleCardRules::DropRequest request;
    request.card_id = drag_context.deck_index;
    request.origin = drag_context.from_board
        ? BattleCardRules::DragOrigin::Board
        : BattleCardRules::DragOrigin::Hand;
    request.candidate_anchor = { candidate_anchor.x, candidate_anchor.y };
    request.pointer_on_board = (ScreenToBoardCell(release_cursor) != Point{ -1,-1 });
	request.near_start = (drag_context.rotation_steps == 0)
		&& (DistanceSquared(release_screen_pos, drag_context.start_screen_pos)
			<= cell_size * cell_size / 4.0);
    for (int32 y = 0; y < selected_block.Size().second; y++) {
        for (int32 x = 0; x < selected_block.Size().first; x++) {
            if (selected_block.GetPiece(x, y).content == '$') continue;
            request.footprint.push_back({ x,y });
        }
    }

    const BattleCardRules::DropDecision decision = BattleCardRules::ResolveDrop(request, snapshot);
    plan.type = decision.result;
    plan.anchor = { decision.anchor.x, decision.anchor.y };
    plan.target_deck_index = decision.target_card_id;
    switch (decision.result) {
    case BattleCardRules::DropResult::Swap: {
        const int32 target_index = FindBoardBlockIndex(plan.target_deck_index);
        if (!IsBoardBlockIndexValid(target_index)
            || !BattleCardRules::CanBeHandSwapTarget(board_blocks[target_index].lifecycle)) {
            plan.type = DropType::ReturnToHand;
        }
        break;
    }
	case BattleCardRules::DropResult::BoardSwap: {
		if (!drag_context.from_board) break;
		const int32 target_index = FindBoardBlockIndex(plan.target_deck_index);
		if (!IsBoardBlockPlaced(target_index)
			|| !BattleCardRules::CanBeHandSwapTarget(board_blocks[target_index].lifecycle)) {
			plan.type = DropType::RestoreToBoard;
			break;
		}
		BattleCardRules::BoardSwapRequest swap_request;
		swap_request.first_card_id = drag_context.deck_index;
		swap_request.first_destination_anchor = {
			board_blocks[target_index].board_anchor.x, board_blocks[target_index].board_anchor.y };
		for (int32 y = 0; y < selected_block.Size().second; ++y) {
			for (int32 x = 0; x < selected_block.Size().first; ++x) {
				if (selected_block.GetPiece(x, y).content != '$') {
					swap_request.first_footprint.push_back({ x, y });
				}
			}
		}
		const BoardBlockState& target = board_blocks[target_index];
		swap_request.second_card_id = target.deck_index;
		swap_request.second_destination_anchor = {
			drag_context.board_anchor.x, drag_context.board_anchor.y };
		for (int32 y = 0; y < target.block->Size().second; ++y) {
			for (int32 x = 0; x < target.block->Size().first; ++x) {
				if (target.block->GetPiece(x, y).content != '$') {
					swap_request.second_footprint.push_back({ x, y });
				}
			}
		}
		if (BattleCardRules::CanSwapBoardCards(swap_request, snapshot)) {
			plan.type = DropType::BoardSwap;
			plan.anchor = target.board_anchor;
		} else {
			plan.type = DropType::RestoreToBoard;
		}
		break;
	}
    default:
        break;
    }
    return plan;
}

void Board::ClearBoardBlock(int32 index) {
    assert(IsBoardBlockIndexValid(index));
    const int32 usage_value = board_blocks[index].deck_index + 1;
    for (size_t y = 0; y < board_usage.height(); y++) {
        for (size_t x = 0; x < board_usage.width(); x++) {
            if (board_usage[y][x] != usage_value) continue;
            board_usage[y][x] = 0;
            board_number[y][x] = 0;
            board_effect_back[y][x] = 0;
            board_content[y][x] = '\0';
        }
    }
    board_blocks[index].board_anchor = { -1,-1 };
}

bool Board::HasBoardOccupancy(int32 index) const {
	if (!IsBoardBlockIndexValid(index)) return false;
	const int32 usage_value = board_blocks[index].deck_index + 1;
	for (const auto& usage : board_usage) {
		if (usage == usage_value) return true;
	}
	return false;
}

void Board::SetBoardBlockPosition(int32 index, Point anchor) {
    if (!IsBoardBlockIndexValid(index)) return;
    BoardBlockState& state = board_blocks[index];
    const Point screen_pos = GetBoardBlockScreenPosition(*state.block, anchor);
    state.block->SetPos(screen_pos.x, screen_pos.y);
    state.board_anchor = anchor;
}

void Board::QueueZoneChange(const int32 deck_index, const GameStateRules::CardZone expected,
	const GameStateRules::CardZone destination) {
	if ((deck_index < 0) || (expected == destination)) return;
	pending_zone_changes.push_back({ deck_index, expected, destination });
}

void Board::SetBlockRotation(int32 index, int32 rotation) {
    BoardBlockState& state = board_blocks[index];
    while (state.rotation != rotation) {
        state.block->Rotate();
        state.rotation = (state.rotation + 1) % 4;
	}
}

void Board::RotateDraggedBlock() {
	const int32 selected_index = ResolveDragBlockIndex();
	BoardBlockState& selected = board_blocks[selected_index];
	selected.block->Rotate();
	selected.rotation = (selected.rotation + 1) % 4;
	drag_context.rotation_steps = (drag_context.rotation_steps + 1) % 4;
}

void Board::SwapBoardBlocks(const int32 selected_index, const int32 target_index) {
	BoardBlockState& selected = board_blocks[selected_index];
	BoardBlockState& target = board_blocks[target_index];
	const Point selected_anchor = drag_context.board_anchor;
	const Point target_anchor = target.board_anchor;
	ClearBoardBlock(selected_index);
	ClearBoardBlock(target_index);
	UpdateBoardNum(selected_index, target_anchor);
	UpdateBoardNum(target_index, selected_anchor);
	SetBoardBlockPosition(selected_index, target_anchor);
	SetBoardBlockPosition(target_index, selected_anchor);
	selected.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
	target.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
	selected.visual_motion = {};
	target.visual_motion = {};
}

void Board::StartVisualReturn(int32 index, BattleCardRules::CardLifecycle lifecycle, Point end_pos) {
	if (!IsBoardBlockIndexValid(index)) return;
	BoardBlockState& state = board_blocks[index];
	const Point start_pos = { state.block->GetPos().first, state.block->GetPos().second };
	state.lifecycle = lifecycle;
	state.motion_start = start_pos;
	state.motion_end = end_pos;
	if (start_pos == end_pos) {
		state.visual_motion = {};
		state.block->SetPos(end_pos.x, end_pos.y);
		state.lifecycle = (lifecycle == BattleCardRules::CardLifecycle::ReturningToHand)
			? BattleCardRules::CardLifecycle::InHand : BattleCardRules::CardLifecycle::OnBoard;
		return;
	}
	state.visual_motion.Start();
}

void Board::ReturnDraggedBlockToHand() {
	assert(IsDragContextValid());
	const int32 selected_index = ResolveDragBlockIndex();
	BoardBlockState& selected = board_blocks[selected_index];
	const bool occupancy_changed = HasBoardOccupancy(selected_index);
	const Point return_pos = selected.hand_pos;
	ClearBoardBlock(selected_index);
	SetBlockRotation(selected_index, 0);
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToHand, return_pos);
	if (drag_context.from_board) {
		QueueZoneChange(selected.deck_index, GameStateRules::CardZone::Board,
			GameStateRules::CardZone::Hand);
	}
	if (occupancy_changed) CalcRow();
	ClearDrag();
}

void Board::RestoreDraggedBlockToBoard() {
	// ドラッグ中も元のマスは占有したままなので，回転と表示位置だけ戻す．
	assert(IsDragContextValid() && drag_context.from_board);
	const int32 selected_index = ResolveDragBlockIndex();
	BoardBlockState& selected = board_blocks[selected_index];
	SetBlockRotation(selected_index, drag_context.start_rotation);
	const Point board_pos = GetBoardBlockScreenPosition(*selected.block, selected.board_anchor);
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToBoard, board_pos);
	ClearDrag();
}

void Board::RollbackDraggedBlock() {
	if (!drag_context.active) return;
	if (drag_context.from_board) RestoreDraggedBlockToBoard();
	else ReturnDraggedBlockToHand();
}

void Board::ClearDrag() {
    drag_context = {};
}

void Board::PutBlock(Point release_cursor, Point release_screen_pos) {
	assert(IsDragContextValid());
	DropPlan plan = AnalyzeDrop(PutBlockAt(release_screen_pos), release_cursor, release_screen_pos);
	const int32 selected_index = ResolveDragBlockIndex();
	BoardBlockState& selected = board_blocks[selected_index];
	if ((plan.type == DropType::ReturnToHand) && (drag_context.rotation_steps == 0)) {
		const int32 original_rotation = selected.rotation;
		RotateDraggedBlock();
		const DropPlan rotated = AnalyzeDrop(PutBlockAt(release_screen_pos), release_cursor, release_screen_pos);
		if (BattleCardRules::ShouldUseAutoRotatedPlacement(plan.type, rotated.type)) {
			plan = rotated;
		} else {
			SetBlockRotation(selected_index, original_rotation);
			drag_context.rotation_steps = 0;
		}
	}
	switch (plan.type) {
	case DropType::ReturnToHand:
		ReturnDraggedBlockToHand();
		return;
	case DropType::RestoreToBoard:
		RestoreDraggedBlockToBoard();
		return;
	case DropType::Place:
		ClearBoardBlock(selected_index);
		UpdateBoardNum(selected_index, plan.anchor);
		SetBoardBlockPosition(selected_index, plan.anchor);
		selected.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
		selected.visual_motion = {};
		if (!drag_context.from_board) {
			QueueZoneChange(selected.deck_index, GameStateRules::CardZone::Hand, GameStateRules::CardZone::Board);
		}
		break;
	case DropType::Swap: {
		const int32 target_index = FindBoardBlockIndex(plan.target_deck_index);
		BoardBlockState& target = board_blocks[target_index];
		ClearBoardBlock(target_index);
		ClearBoardBlock(selected_index);
		UpdateBoardNum(selected_index, plan.anchor);
		SetBoardBlockPosition(selected_index, plan.anchor);
		std::swap(selected.hand_pos, target.hand_pos);
		std::swap(selected.hand_slot, target.hand_slot);
		selected.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
		selected.visual_motion = {};
		SetBlockRotation(target_index, 0);
		StartVisualReturn(target_index, BattleCardRules::CardLifecycle::ReturningToHand, target.hand_pos);
		QueueZoneChange(selected.deck_index, GameStateRules::CardZone::Hand, GameStateRules::CardZone::Board);
		QueueZoneChange(target.deck_index, GameStateRules::CardZone::Board, GameStateRules::CardZone::Hand);
		break;
	}
	case DropType::BoardSwap:
		SwapBoardBlocks(selected_index, FindBoardBlockIndex(plan.target_deck_index));
		break;
	}
	CalcRow();
	ClearDrag();
}

void Board::TakeOutBlock(Point pos, Point cursor_pos) {//クリックしたBlockのドラッグを開始する
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    if (drag_context.active
        || (pos.x < 0) || (board_width <= pos.x)
        || (pos.y < 0) || (board_height <= pos.y)) return;
    const int32 usage = board_usage[pos.y][pos.x];
    if (usage <= 0) return;
    const int32 selected_index = FindBoardBlockIndex(usage - 1);
	if (!IsBoardBlockPlaced(selected_index)) return;
	BoardBlockState& selected = board_blocks[selected_index];
	if (!BattleCardRules::CanStartCardDrag(selected.lifecycle)) return;
    drag_context.active = true;
    drag_context.deck_index = selected.deck_index;
    drag_context.from_board = true;
    drag_context.start_screen_pos = { selected.block->GetPos().first, selected.block->GetPos().second };
    drag_context.board_anchor = selected.board_anchor;
    drag_context.cursor_offset = drag_context.start_screen_pos - cursor_pos;
	drag_context.start_rotation = selected.rotation;
	selected.visual_motion = {};
	selected.lifecycle = BattleCardRules::CardLifecycle::DraggingFromBoard;
}

void Board::InitBoardCoordinate() {//board_coordinateの初期化
    for (int i = 0;i < 6;i++) {
        for (int j = 0;j < 7;j++) {
            board_coordinate[i][j] = GetBoardCellCenter({ j,i });
        }
    }
}

void Board::DoRelic(const vector<int32>& relics) {
	assert(relics.size() == 19);
	for (int i = 0; i < 6; i++) {
		board_multiply[i] = board_multiply_base[i] + relics[3] * 0.5;//ID3の効果で行倍率を0.5ずつ加算する．
	}
	const int32 new_off_count = 3 + relics[10] - relics[11];//基本の攻撃列3行をレリックで増減する．
    if (off_count != new_off_count) {
        off_count = new_off_count;
        RebuildBoardDerivedState();
    }
	add_damage = relics[13] * 3;//ID13の効果を3倍して攻撃補正にする．
	add_armor = relics[14] * 3;//ID14の効果を3倍して防御補正にする．
	do_armor_raise = (relics[15] == 1);
	add_damage_by_cards = relics[16];
}

bool Board::RegisterHandBlock(Block& block, int32 deck_index, int32 hand_slot, Point hand_pos) {
	if ((deck_index < 0) || (hand_slot < 0) || (hand_pos == Point{ -1,-1 })) return false;
	const int32 existing_index = FindBoardBlockIndex(deck_index);
	if (0 <= existing_index) return IsBoardBlockIndexValid(existing_index)
		&& (board_blocks[existing_index].block == &block)
		&& BattleCardRules::IsLogicallyInHand(board_blocks[existing_index].lifecycle);
	for (const auto& state : board_blocks) {
		if ((state.block == &block) || (state.hand_slot == hand_slot) || (state.hand_pos == hand_pos)) return false;
	}
	BoardBlockState state;
	state.block = &block;
	state.deck_index = deck_index;
	state.hand_slot = hand_slot;
	state.hand_pos = hand_pos;
	state.lifecycle = BattleCardRules::CardLifecycle::InHand;
	board_blocks.push_back(state);
	return true;
}

bool Board::PassBlock(Block& selectedBlock, int32 deck_index, Point cursor_pos) {//選択されているBlockとそのDeck番号が渡される
	if (drag_context.active || (deck_index < 0)) return false;
	const int32 selected_index = FindBoardBlockIndex(deck_index);
	if (!IsBoardBlockIndexValid(selected_index)) return false;
	BoardBlockState& selected = board_blocks[selected_index];
	if ((selected.block != &selectedBlock)
		|| (selected.lifecycle != BattleCardRules::CardLifecycle::InHand)
		|| (selected.board_anchor != Point{ -1,-1 }) || selected.visual_motion.active) return false;
	drag_context.active = true;
    drag_context.deck_index = deck_index;
	drag_context.from_board = false;
	drag_context.start_screen_pos = selected.hand_pos;
	drag_context.board_anchor = { -1,-1 };
	drag_context.cursor_offset = selected.hand_pos - cursor_pos;
	drag_context.start_rotation = selected.rotation;
	selected.lifecycle = BattleCardRules::CardLifecycle::DraggingFromHand;
	selected.visual_motion = {};
	return true;
}
