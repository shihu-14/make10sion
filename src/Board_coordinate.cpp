#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
#include "BattleCardRules.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using namespace std;

//private variables
int32 Board::FindBoardBlockIndex(int32 deck_index) const {
    for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
        if (board_blocks[i].deck_index == deck_index) return i;
    }
    return -1;
}

int32 Board::ResolveDragBlockIndex() const {
    if (!drag_context.active || (drag_context.block == nullptr)) return -1;
    const int32 selected_index = drag_context.board_block_index;
    if (IsBoardBlockIndexValid(selected_index)
        && (board_blocks[selected_index].deck_index == drag_context.deck_index)
        && (board_blocks[selected_index].block == drag_context.block)) return selected_index;

    const int32 resolved_index = FindBoardBlockIndex(drag_context.deck_index);
    if (IsBoardBlockIndexValid(resolved_index)) return resolved_index;
    for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
        if (IsBoardBlockIndexValid(i) && (board_blocks[i].block == drag_context.block)) return i;
    }
    return -1;
}

bool Board::IsBoardBlockIndexValid(int32 index) const {
    return (0 <= index)
        && (index < static_cast<int32>(board_blocks.size()))
        && (board_blocks[index].block != nullptr)
        && (0 <= board_blocks[index].deck_index);
}

bool Board::IsDragContextValid() const {
	if (!drag_context.active || (drag_context.block == nullptr)
		|| !IsBoardBlockIndexValid(drag_context.board_block_index)
		|| (board_blocks[drag_context.board_block_index].deck_index != drag_context.deck_index)
		|| (board_blocks[drag_context.board_block_index].block != drag_context.block)) return false;
	const auto lifecycle = board_blocks[drag_context.board_block_index].lifecycle;
	return drag_context.from_board
		? (lifecycle == BattleCardRules::CardLifecycle::DraggingFromBoard)
		: (lifecycle == BattleCardRules::CardLifecycle::DraggingFromHand);
}

Point Board::GetBoardCellCenter(Point cell) const {
    return offset + Point{
        cell.x * cell_size + cell_size / 2,
        cell.y * cell_size + cell_size / 2
    };
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
    const Point relative_pos = screen_pos - offset;
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    if ((relative_pos.x < 0) || (board_width * cell_size <= relative_pos.x)
        || (relative_pos.y < 0) || (board_height * cell_size <= relative_pos.y)) return { -1,-1 };
    return relative_pos / cell_size;
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
    if (state.block->GetStat() != 2) return false;
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

bool Board::CanPlaceBlock(int32 index, Point anchor, int32 ignored_index_1, int32 ignored_index_2) const {
    if (!IsBoardBlockIndexValid(index) || (anchor == Point{ -1,-1 })) return false;
    Array<Point> cells;
    if (!GetBlockCells(*board_blocks[index].block, anchor, cells)) return false;
    for (const auto& cell : cells) {
        const int32 usage = board_usage[cell.y][cell.x];
        if (usage < 0) return false;
        if (usage == 0) continue;
        const int32 occupied_deck_index = usage - 1;
        const int32 ignored_deck_index_1 = IsBoardBlockIndexValid(ignored_index_1)
            ? board_blocks[ignored_index_1].deck_index : -1;
        const int32 ignored_deck_index_2 = IsBoardBlockIndexValid(ignored_index_2)
            ? board_blocks[ignored_index_2].deck_index : -1;
        if ((occupied_deck_index != ignored_deck_index_1)
            && (occupied_deck_index != ignored_deck_index_2)) return false;
    }
    return true;
}

bool Board::ValidateBoardState(int32 allowed_target_index, String* diagnostic) const {
	(void)allowed_target_index;
	const auto fail = [diagnostic](const String& message) {
		if (diagnostic) *diagnostic = message;
		return false;
	};
	int32 dragging_count = 0;
	int32 dragging_index = -1;
	for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
		if (!IsBoardBlockIndexValid(i)) return fail(U"invalid board_blocks entry: index=" + Format(i));
		const BoardBlockState& state = board_blocks[i];
		if ((state.lifecycle == BattleCardRules::CardLifecycle::DraggingFromHand)
			|| (state.lifecycle == BattleCardRules::CardLifecycle::DraggingFromBoard)) {
			dragging_count++;
			dragging_index = i;
		}
		if ((state.hand_slot < 0) || (state.hand_pos == Point{ -1,-1 })) {
			return fail(U"missing reserved hand position: deck_index=" + Format(board_blocks[i].deck_index));
		}
		for (int32 j = 0; j < i; j++) {
            if (board_blocks[i].deck_index == board_blocks[j].deck_index) {
                return fail(U"duplicate deck_index=" + Format(board_blocks[i].deck_index));
            }
            if (board_blocks[i].block == board_blocks[j].block) {
                return fail(U"duplicate Block pointer: deck_index=" + Format(board_blocks[i].deck_index));
            }
			if (board_blocks[i].hand_pos == board_blocks[j].hand_pos) {
				return fail(U"duplicate reserved hand position=" + Format(board_blocks[i].hand_pos));
			}
			if (board_blocks[i].hand_slot == board_blocks[j].hand_slot) {
				return fail(U"duplicate reserved hand slot=" + Format(board_blocks[i].hand_slot));
			}
		}
	}
	if (drag_context.active) {
		if ((dragging_count != 1) || (dragging_index != ResolveDragBlockIndex())
			|| !IsDragContextValid()) {
			return fail(U"active drag/lifecycle mismatch: drag_deck_index="
				+ Format(drag_context.deck_index) + U", dragging_count=" + Format(dragging_count));
		}
	} else if (dragging_count != 0) {
		return fail(U"dragging lifecycle without active context: index=" + Format(dragging_index));
	}

    for (int32 y = 0; y < static_cast<int32>(board_usage.height()); y++) {
        for (int32 x = 0; x < static_cast<int32>(board_usage.width()); x++) {
			const int32 usage = board_usage[y][x];
			if (usage <= 0) continue;
			const int32 index = FindBoardBlockIndex(usage - 1);
			if (!IsBoardBlockIndexValid(index)
				|| !BattleCardRules::IsLogicallyOnBoard(board_blocks[index].lifecycle)
				|| (board_blocks[index].block->GetStat() != 2)) {
				return fail(U"orphan board_usage: cell=" + Format(Point{ x,y }) + U", value=" + Format(usage));
			}
        }
    }

    for (int32 index = 0; index < static_cast<int32>(board_blocks.size()); index++) {
        const BoardBlockState& state = board_blocks[index];
        size_t occupied_count = 0;
        for (const auto& usage : board_usage) {
            if (usage == state.deck_index + 1) occupied_count++;
        }

		if (BattleCardRules::IsLogicallyInHand(state.lifecycle)) {
			if ((state.block->GetStat() != 1) || (state.board_anchor != Point{ -1,-1 }) || (occupied_count != 0)) {
				return fail(U"hand card has board state: deck_index=" + Format(state.deck_index));
			}
			if ((state.lifecycle == BattleCardRules::CardLifecycle::ReturningToHand) != state.visual_motion.active) {
				return fail(U"hand return motion mismatch: deck_index=" + Format(state.deck_index));
			}
			if (state.lifecycle == BattleCardRules::CardLifecycle::ReturningToHand) {
				const Point motion_end = { state.visual_motion.end.x, state.visual_motion.end.y };
				if (motion_end != state.hand_pos) {
					return fail(U"hand return endpoint mismatch: deck_index=" + Format(state.deck_index));
				}
			}
			if (state.lifecycle == BattleCardRules::CardLifecycle::InHand) {
				const Point screen_pos = { state.block->GetPos().first, state.block->GetPos().second };
				if (screen_pos != state.hand_pos) return fail(U"hand screen position mismatch: deck_index=" + Format(state.deck_index));
			}
			continue;
		}
		if (!BattleCardRules::IsLogicallyOnBoard(state.lifecycle) || (state.block->GetStat() != 2)) {
			return fail(U"card lifecycle/stat mismatch: deck_index=" + Format(state.deck_index));
		}
		if (state.lifecycle == BattleCardRules::CardLifecycle::DraggingFromBoard) continue;
		Array<Point> expected_cells;
		if (!GetBlockCells(*state.block, state.board_anchor, expected_cells)
			|| (occupied_count != expected_cells.size())
			|| !IsBoardBlockPlaced(index)) {
            return fail(U"board footprint mismatch: deck_index=" + Format(state.deck_index)
                + U", anchor=" + Format(state.board_anchor));
        }
        const Point screen_pos = { state.block->GetPos().first, state.block->GetPos().second };
		const Point expected_screen_pos = GetBoardBlockScreenPosition(*state.block, state.board_anchor);
		if ((state.lifecycle == BattleCardRules::CardLifecycle::ReturningToBoard) != state.visual_motion.active) {
			return fail(U"board return motion mismatch: deck_index=" + Format(state.deck_index));
		}
		if (state.lifecycle == BattleCardRules::CardLifecycle::ReturningToBoard) {
			const Point motion_end = { state.visual_motion.end.x, state.visual_motion.end.y };
			if (motion_end != expected_screen_pos) {
				return fail(U"board return endpoint mismatch: deck_index=" + Format(state.deck_index));
			}
		}
		if ((state.lifecycle == BattleCardRules::CardLifecycle::OnBoard) && (screen_pos != expected_screen_pos)) {
			return fail(U"board screen position mismatch: deck_index=" + Format(state.deck_index)
				+ U", actual=" + Format(screen_pos));
		}
		if (state.lifecycle == BattleCardRules::CardLifecycle::ReturningToBoard) continue;
		for (int32 y = 0; y < state.block->Size().second; y++) {
            for (int32 x = 0; x < state.block->Size().first; x++) {
                const Piece& piece = state.block->GetPiece(x, y);
                if (piece.content == '$') continue;
                const Point piece_screen_pos = screen_pos + GetScaledPieceOffset(piece);
                if (piece_screen_pos != GetBoardCellCenter(state.board_anchor + Point{ x,y })) {
                    return fail(U"piece position mismatch: deck_index=" + Format(state.deck_index)
                        + U", cell=" + Format(state.board_anchor + Point{ x,y }));
                }
            }
        }
    }
	return true;
}

void Board::AssertBoardState(int32 allowed_target_index) const {
#ifndef NDEBUG
	String diagnostic;
	if (!ValidateBoardState(allowed_target_index, &diagnostic)) {
		Logger << U"Board invariant violation: " << diagnostic;
		DumpInteractionState(U"board-invariant");
		assert(false && "Board invariant violation; see Logger output");
    }
#else
    (void)allowed_target_index;
#endif
}

void Board::TraceTransition(StringView event, int32 deck_index, StringView detail) {
#ifndef NDEBUG
	String line = U"frame=" + Format(current_frame_number) + U", event=" + String{ event }
		+ U", deck_index=" + Format(deck_index);
	if (!detail.isEmpty()) line += U", " + String{ detail };
	interaction_trace.push_back(line);
	if (128 < interaction_trace.size()) interaction_trace.pop_front();
#else
	(void)event;
	(void)deck_index;
	(void)detail;
#endif
}

void Board::DumpInteractionState(StringView context) const {
#ifndef NDEBUG
	Logger << U"Board interaction dump: " << context << U", frame=" << current_frame_number
		<< U", drag_active=" << drag_context.active << U", drag_deck_index=" << drag_context.deck_index;
	for (const auto& state : board_blocks) {
		const Point screen_pos = state.block
			? Point{ state.block->GetPos().first, state.block->GetPos().second } : Point{ -1,-1 };
		Logger << U"card deck_index=" << state.deck_index << U", lifecycle="
			<< static_cast<int32>(state.lifecycle) << U", stat=" << (state.block ? state.block->GetStat() : -99)
			<< U", screen=" << screen_pos << U", hand_slot=" << state.hand_slot
			<< U", hand_pos=" << state.hand_pos << U", anchor=" << state.board_anchor
			<< U", rotation=" << state.rotation << U", motion_active=" << state.visual_motion.active
			<< U", motion_start=" << Point{ state.visual_motion.start.x, state.visual_motion.start.y }
			<< U", motion_end=" << Point{ state.visual_motion.end.x, state.visual_motion.end.y }
			<< U", motion_current=" << Point{ state.visual_motion.current.x, state.visual_motion.current.y }
			<< U", motion_elapsed=" << state.visual_motion.elapsed_seconds;
	}
	for (int32 y = 0; y < static_cast<int32>(board_usage.height()); y++) {
		String row;
		for (int32 x = 0; x < static_cast<int32>(board_usage.width()); x++) row += Format(board_usage[y][x]) + U" ";
		Logger << U"board_usage[" << y << U"] " << row;
	}
	for (const auto& line : interaction_trace) Logger << U"trace " << line;
#else
	(void)context;
#endif
}

Point Board::PutBlockAt(Point screen_pos) const {//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す
    if (!IsDragContextValid()) return { -1,-1 };
    const Block& block = *board_blocks[drag_context.board_block_index].block;
    return GetBoardAnchorFromScreenPosition(block, screen_pos);
}

Board::DropPlan Board::AnalyzeDrop(Point candidate_anchor, Point release_cursor, Point release_screen_pos) const {
    DropPlan plan;
    if (!IsDragContextValid()) return plan;
    const int32 selected_index = drag_context.board_block_index;
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
    request.original_anchor = { drag_context.board_anchor.x, drag_context.board_anchor.y };
    request.pointer_on_board = (ScreenToBoardCell(release_cursor) != Point{ -1,-1 });
    request.near_start = CalcDist(release_screen_pos, drag_context.start_screen_pos)
        <= static_cast<double>(cell_size * cell_size) / 4.0;
    for (int32 y = 0; y < selected_block.Size().second; y++) {
        for (int32 x = 0; x < selected_block.Size().first; x++) {
            if (selected_block.GetPiece(x, y).content == '$') continue;
            request.footprint.push_back({ x,y });
        }
    }

    const BattleCardRules::DropDecision decision = BattleCardRules::ResolveDrop(request, snapshot);
    plan.anchor = { decision.anchor.x, decision.anchor.y };
    plan.target_deck_index = decision.target_card_id;
    switch (decision.result) {
    case BattleCardRules::DropResult::ReturnToHand:
        plan.type = DropType::ReturnToHand;
        break;
    case BattleCardRules::DropResult::RestoreToBoard:
        plan.type = DropType::RestoreToBoard;
        break;
    case BattleCardRules::DropResult::Place:
        if (CanPlaceBlock(selected_index, plan.anchor, selected_index)) plan.type = DropType::Place;
        break;
    case BattleCardRules::DropResult::Swap: {
        const int32 target_index = FindBoardBlockIndex(plan.target_deck_index);
        if (IsBoardBlockPlaced(target_index)
			&& BattleCardRules::CanBeHandSwapTarget(board_blocks[target_index].lifecycle)
            && (board_blocks[target_index].hand_pos != Point{ -1,-1 })
            && (drag_context.hand_pos != Point{ -1,-1 })
            && (board_blocks[target_index].hand_pos != drag_context.hand_pos)
            && CanPlaceBlock(selected_index, plan.anchor, selected_index, target_index)) {
            plan.type = DropType::Swap;
        }
        break;
    }
    }
    return plan;
}

bool Board::ClearBoardBlock(int32 index) {
    if (!IsBoardBlockIndexValid(index)) return false;
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
    return true;
}

bool Board::HasBoardOccupancy(int32 index) const {
	if (!IsBoardBlockIndexValid(index)) return false;
	const int32 usage_value = board_blocks[index].deck_index + 1;
	for (const auto& usage : board_usage) {
		if (usage == usage_value) return true;
	}
	return false;
}

bool Board::ForceOrphanedDragToHand() {
	if (!drag_context.active) return false;
	int32 existing_index = FindBoardBlockIndex(drag_context.deck_index);
	if (!IsBoardBlockIndexValid(existing_index) && drag_context.block) {
		for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
			if (IsBoardBlockIndexValid(i) && (board_blocks[i].block == drag_context.block)) {
				existing_index = i;
				break;
			}
		}
	}
	Block* block = drag_context.block;
	if (!block && IsBoardBlockIndexValid(existing_index)) block = board_blocks[existing_index].block;
	if (!block) {
		DumpInteractionState(U"orphaned-drag-without-card");
		ClearDrag();
		return false;
	}

	DumpInteractionState(U"orphaned-drag-recovery");
	const int32 deck_index = (0 <= drag_context.deck_index) ? drag_context.deck_index
		: (IsBoardBlockIndexValid(existing_index) ? board_blocks[existing_index].deck_index : -1);
	const int32 hand_slot = (0 <= drag_context.hand_slot) ? drag_context.hand_slot
		: (IsBoardBlockIndexValid(existing_index) ? board_blocks[existing_index].hand_slot : -1);
	Point hand_pos = drag_context.from_board ? drag_context.hand_pos : drag_context.start_screen_pos;
	if ((hand_pos == Point{ -1,-1 }) && IsBoardBlockIndexValid(existing_index)) {
		hand_pos = board_blocks[existing_index].hand_pos;
	}
	int32 rotation = IsBoardBlockIndexValid(existing_index) ? board_blocks[existing_index].rotation
		: ((drag_context.start_rotation + drag_context.rotation_steps) % 4 + 4) % 4;
	const int32 usage_value = deck_index + 1;
	if (0 <= deck_index) {
		for (size_t y = 0; y < board_usage.height(); y++) {
			for (size_t x = 0; x < board_usage.width(); x++) {
				if (board_usage[y][x] != usage_value) continue;
				board_usage[y][x] = 0;
				board_number[y][x] = 0;
				board_effect_back[y][x] = 0;
				board_content[y][x] = '\0';
			}
		}
	}
	for (int32 i = static_cast<int32>(board_blocks.size()) - 1; 0 <= i; --i) {
		if ((board_blocks[i].deck_index == deck_index) || (board_blocks[i].block == block)) {
			board_blocks.erase(board_blocks.begin() + i);
		}
	}

	while (rotation != 0) {
		block->Rotate();
		rotation = (rotation + 1) % 4;
	}
	if (hand_pos != Point{ -1,-1 }) block->SetPos(hand_pos.x, hand_pos.y);
	block->SetStat(1);

	if ((deck_index >= 0) && (hand_slot >= 0) && (hand_pos != Point{ -1,-1 })) {
		BoardBlockState recovered;
		recovered.block = block;
		recovered.deck_index = deck_index;
		recovered.hand_slot = hand_slot;
		recovered.hand_pos = hand_pos;
		recovered.lifecycle = BattleCardRules::CardLifecycle::InHand;
		board_blocks.push_back(recovered);
	}
	CalcRow();
	TraceTransition(U"orphaned-drag-returned", deck_index);
	const bool recovered = (deck_index >= 0) && (hand_slot >= 0) && (hand_pos != Point{ -1,-1 });
	ClearDrag();
	if (recovered) AssertBoardState();
	return recovered;
}

void Board::SetBoardBlockPosition(int32 index, Point anchor) {
    if (!IsBoardBlockIndexValid(index)) return;
    BoardBlockState& state = board_blocks[index];
    const Point screen_pos = GetBoardBlockScreenPosition(*state.block, anchor);
    state.block->SetPos(screen_pos.x, screen_pos.y);
    state.board_anchor = anchor;
}

void Board::SetBlockRotation(int32 index, int32 rotation) {
    if (!IsBoardBlockIndexValid(index)) return;
    BoardBlockState& state = board_blocks[index];
    const int32 target_rotation = ((rotation % 4) + 4) % 4;
    while (state.rotation != target_rotation) {
        state.block->Rotate();
        state.rotation = (state.rotation + 1) % 4;
	}
}

void Board::StartVisualReturn(int32 index, BattleCardRules::CardLifecycle lifecycle, Point end_pos) {
	if (!IsBoardBlockIndexValid(index)) return;
	BoardBlockState& state = board_blocks[index];
	const Point start_pos = { state.block->GetPos().first, state.block->GetPos().second };
	state.lifecycle = lifecycle;
	if (start_pos == end_pos) {
		state.visual_motion = {};
		state.block->SetPos(end_pos.x, end_pos.y);
		state.lifecycle = (lifecycle == BattleCardRules::CardLifecycle::ReturningToHand)
			? BattleCardRules::CardLifecycle::InHand : BattleCardRules::CardLifecycle::OnBoard;
		return;
	}
	BattleCardRules::StartVisualMotion(state.visual_motion,
		{ start_pos.x, start_pos.y }, { end_pos.x, end_pos.y });
}

bool Board::ReturnDraggedBlockToHand() {
	if (!drag_context.active) return false;
	const int32 selected_index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(selected_index)) {
		return ForceOrphanedDragToHand();
	}
	const Point return_pos = drag_context.from_board ? drag_context.hand_pos : drag_context.start_screen_pos;
	if (return_pos == Point{ -1,-1 }) return RollbackDraggedBlock();
	const bool occupancy_changed = HasBoardOccupancy(selected_index);
	BoardBlockState& selected = board_blocks[selected_index];
	if (!ClearBoardBlock(selected_index)) return RollbackDraggedBlock();
	SetBlockRotation(selected_index, 0);
	selected.board_anchor = { -1,-1 };
	selected.block->SetStat(1);
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToHand, return_pos);
	if (occupancy_changed) CalcRow();
	TraceTransition(U"return-to-hand", selected.deck_index,
		drag_context.from_board ? U"origin=board" : U"origin=hand");
	ClearDrag();
	AssertBoardState();
	return true;
}

bool Board::RestoreDraggedBlockToBoard() {
	if (!drag_context.active || !drag_context.from_board) return false;
	const int32 selected_index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(selected_index)) return ForceOrphanedDragToHand();
	BoardBlockState& selected = board_blocks[selected_index];
	SetBlockRotation(selected_index, drag_context.start_rotation);
	selected.board_anchor = drag_context.board_anchor;
	selected.block->SetStat(2);
	if (!IsBoardBlockPlaced(selected_index)) return RestoreDraggedBlockAfterFailedCommit();

	const Point board_pos = GetBoardBlockScreenPosition(*selected.block, selected.board_anchor);
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToBoard, board_pos);
	TraceTransition(U"restore-to-board", selected.deck_index, U"occupancy=preserved");
	ClearDrag();
	AssertBoardState();
	return true;
}

bool Board::RestoreDraggedBlockAfterFailedCommit() {
	if (!drag_context.active || !drag_context.from_board) return false;
	const int32 selected_index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(selected_index)) return ForceOrphanedDragToHand();
	BoardBlockState& selected = board_blocks[selected_index];
	ClearBoardBlock(selected_index);
	SetBlockRotation(selected_index, drag_context.start_rotation);
	selected.board_anchor = drag_context.board_anchor;
	if ((selected.board_anchor != Point{ -1,-1 })
		&& CanPlaceBlock(selected_index, selected.board_anchor, selected_index)
		&& UpdateBoardNum(selected_index, selected.board_anchor)) {
		selected.block->SetStat(2);
		const Point board_pos = GetBoardBlockScreenPosition(*selected.block, selected.board_anchor);
		StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToBoard, board_pos);
		CalcRow();
		TraceTransition(U"rollback-rebuild-board", selected.deck_index);
		ClearDrag();
		AssertBoardState();
		return true;
	}

	ClearBoardBlock(selected_index);
	SetBlockRotation(selected_index, 0);
	selected.block->SetStat(1);
	selected.board_anchor = { -1,-1 };
	if (drag_context.hand_pos == Point{ -1,-1 }) return ForceOrphanedDragToHand();
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToHand,
		drag_context.hand_pos);
	CalcRow();
	TraceTransition(U"rollback-fallback-hand", selected.deck_index);
	ClearDrag();
	AssertBoardState();
	return false;
}

bool Board::RollbackDraggedBlock() {
	if (!drag_context.active) return false;
	const int32 selected_index = ResolveDragBlockIndex();
	if (!IsBoardBlockIndexValid(selected_index)) {
		return ForceOrphanedDragToHand();
	}
	if (drag_context.from_board) return RestoreDraggedBlockToBoard();

	BoardBlockState& selected = board_blocks[selected_index];
	const bool occupancy_changed = HasBoardOccupancy(selected_index);
	ClearBoardBlock(selected_index);
	SetBlockRotation(selected_index, 0);
	selected.block->SetStat(1);
	selected.board_anchor = { -1,-1 };
	if (drag_context.start_screen_pos == Point{ -1,-1 }) return ForceOrphanedDragToHand();
	StartVisualReturn(selected_index, BattleCardRules::CardLifecycle::ReturningToHand,
		drag_context.start_screen_pos);
	if (occupancy_changed) CalcRow();
	TraceTransition(U"rollback-to-hand", selected.deck_index);
	ClearDrag();
	AssertBoardState();
	return true;
}

void Board::ClearDrag() {
    drag_context = {};
}

void Board::PutBlock(Point release_cursor, Point release_screen_pos) {//blockがドロップされたら、配置/交換/手札への復帰を行う
    if (!IsDragContextValid()) {
        RollbackDraggedBlock();
        return;
    }
	const DropPlan plan = AnalyzeDrop(PutBlockAt(release_screen_pos), release_cursor, release_screen_pos);
	const int32 selected_index = drag_context.board_block_index;
	BoardBlockState& selected = board_blocks[selected_index];
	TraceTransition(U"drop-resolved", selected.deck_index,
		U"result=" + Format(static_cast<int32>(plan.type))
		+ U", anchor=" + Format(plan.anchor) + U", target=" + Format(plan.target_deck_index));
    if (plan.type == DropType::ReturnToHand) {
        ReturnDraggedBlockToHand();
        return;
    }
    if (plan.type == DropType::RestoreToBoard) {
        RestoreDraggedBlockToBoard();
        return;
    }

	if (plan.type == DropType::Place) {
		if (!CanPlaceBlock(selected_index, plan.anchor, selected_index)) {
			RollbackDraggedBlock();
			return;
		}
		if (!ClearBoardBlock(selected_index) || !UpdateBoardNum(selected_index, plan.anchor)) {
			if (drag_context.from_board) RestoreDraggedBlockAfterFailedCommit();
			else RollbackDraggedBlock();
			return;
		}
		SetBoardBlockPosition(selected_index, plan.anchor);
		selected.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
		selected.visual_motion = {};
		selected.block->SetStat(2);
    } else if ((plan.type == DropType::Swap) && !drag_context.from_board) {
        const int32 target_index = FindBoardBlockIndex(plan.target_deck_index);
        if (!IsBoardBlockPlaced(target_index)
            || !CanPlaceBlock(selected_index, plan.anchor, selected_index, target_index)) {
            RollbackDraggedBlock();
            return;
        }
		BoardBlockState& target = board_blocks[target_index];
		const Point selected_hand_pos = drag_context.hand_pos;
		const Point target_hand_pos = target.hand_pos;
		const int32 selected_hand_slot = selected.hand_slot;
		const int32 target_hand_slot = target.hand_slot;
		const Point target_anchor = target.board_anchor;
		const int32 target_rotation = target.rotation;
		if ((selected_hand_pos == Point{ -1,-1 }) || (target_hand_pos == Point{ -1,-1 })
			|| (selected_hand_pos == target_hand_pos)
			|| (selected_hand_slot < 0) || (target_hand_slot < 0)
			|| (selected_hand_slot == target_hand_slot)) {
			RollbackDraggedBlock();
			return;
		}
		const bool target_cleared = ClearBoardBlock(target_index);
		const bool selected_cleared = target_cleared && ClearBoardBlock(selected_index);
		const bool selected_placed = selected_cleared && UpdateBoardNum(selected_index, plan.anchor);
		if (!selected_placed) {
			ClearBoardBlock(selected_index);
			SetBlockRotation(target_index, target_rotation);
			target.board_anchor = target_anchor;
			if (UpdateBoardNum(target_index, target_anchor)) {
				SetBoardBlockPosition(target_index, target_anchor);
				target.block->SetStat(2);
				target.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
				target.visual_motion = {};
			} else {
				ClearBoardBlock(target_index);
				SetBlockRotation(target_index, 0);
				target.block->SetStat(1);
				target.board_anchor = { -1,-1 };
				StartVisualReturn(target_index,
					BattleCardRules::CardLifecycle::ReturningToHand, target.hand_pos);
			}
			CalcRow();
			RollbackDraggedBlock();
			return;
		}
		SetBoardBlockPosition(selected_index, plan.anchor);
		selected.hand_pos = target_hand_pos;
		selected.hand_slot = target_hand_slot;
		selected.lifecycle = BattleCardRules::CardLifecycle::OnBoard;
		selected.visual_motion = {};
		selected.block->SetStat(2);
		SetBlockRotation(target_index, 0);
		target.hand_pos = selected_hand_pos;
		target.hand_slot = selected_hand_slot;
		target.board_anchor = { -1,-1 };
		target.block->SetStat(1);
		StartVisualReturn(target_index, BattleCardRules::CardLifecycle::ReturningToHand, target.hand_pos);
		TraceTransition(U"swap-place", selected.deck_index, U"target=" + Format(target.deck_index));
    } else {
        RollbackDraggedBlock();
        return;
    }
	CalcRow();
	if (plan.type == DropType::Place) TraceTransition(U"place", selected.deck_index, U"anchor=" + Format(plan.anchor));
	ClearDrag();
	AssertBoardState(FindBoardBlockIndex(plan.target_deck_index));
}

void Board::TakeOutBlock(Point pos, Point cursor_pos) {//クリックしたBlockのドラッグを開始する
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    if (drag_context.active
        || (pos.x < 0) || (board_width <= pos.x)
        || (pos.y < 0) || (board_height <= pos.y)) return;
    const int32 usage = board_usage[pos.y][pos.x];
    if (usage <= 0) return;
    AssertBoardState();
    const int32 selected_index = FindBoardBlockIndex(usage - 1);
	if (!IsBoardBlockPlaced(selected_index)) return;
	BoardBlockState& selected = board_blocks[selected_index];
	if ((selected.block->GetStat() != 2)
		|| (selected.lifecycle != BattleCardRules::CardLifecycle::OnBoard)) return;
    drag_context.active = true;
    drag_context.board_block_index = selected_index;
    drag_context.deck_index = selected.deck_index;
    drag_context.block = selected.block;
    drag_context.from_board = true;
    drag_context.start_screen_pos = { selected.block->GetPos().first, selected.block->GetPos().second };
    drag_context.hand_pos = selected.hand_pos;
	drag_context.hand_slot = selected.hand_slot;
    drag_context.board_anchor = selected.board_anchor;
    drag_context.cursor_offset = drag_context.start_screen_pos - cursor_pos;
	drag_context.start_rotation = selected.rotation;
	selected.visual_motion = {};
	selected.lifecycle = BattleCardRules::CardLifecycle::DraggingFromBoard;
	TraceTransition(U"drag-start-board", selected.deck_index);
}

void Board::InitBoardCoordinate() {//board_coordinateの初期化
    for (int i = 0;i < 6;i++) {
        for (int j = 0;j < 7;j++) {
            board_coordinate[i][j] = GetBoardCellCenter({ j,i });
        }
    }
}

void Board::DoRelic(vector<int32> relics) { //cf.) md
    if (relics[3] > relics_old[3]) {
        for (int i = 0;i < 6;i++) {
            board_multiply[i] += 0.5;
        }
    }
    const int32 new_off_count = 3 + relics[10] - relics[11];//攻防の範囲の動かす数を記録
    if (off_count != new_off_count) {
        off_count = new_off_count;
        RebuildBoardDerivedState();
    }
    if (relics[13] > relics_old[13]) {
        add_damage += (relics[13] - relics_old[13]) * 3;
    }
    if (relics[14] > relics_old[14]) {
        add_armor = relics[14] * 3;
    }

    do_armor_raise = (relics[15] == 1);

    if (relics[16] > relics_old[16]) {
        add_damage_by_cards = relics[16];
    }
}



//public variables

//public　functions
bool Board::RegisterHandBlock(Block& block, int32 deck_index, int32 hand_slot, Point hand_pos) {
	if ((deck_index < 0) || (hand_slot < 0) || (hand_pos == Point{ -1,-1 }) || (block.GetStat() != 1)) return false;
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
	TraceTransition(U"register-hand", deck_index, U"slot=" + Format(hand_slot));
	AssertBoardState();
	return true;
}

bool Board::PassBlock(Block& selectedBlock, int32 deck_index, Point cursor_pos) {//選択されているBlockとそのDeck番号が渡される
	if (drag_context.active || (deck_index < 0) || (selectedBlock.GetStat() != 1)) return false;
	AssertBoardState();
	const int32 selected_index = FindBoardBlockIndex(deck_index);
	if (!IsBoardBlockIndexValid(selected_index)) return false;
	BoardBlockState& selected = board_blocks[selected_index];
	if ((selected.block != &selectedBlock)
		|| (selected.lifecycle != BattleCardRules::CardLifecycle::InHand)
		|| (selected.board_anchor != Point{ -1,-1 }) || selected.visual_motion.active) return false;
	drag_context.active = true;
    drag_context.board_block_index = selected_index;
    drag_context.deck_index = deck_index;
	drag_context.block = &selectedBlock;
	drag_context.from_board = false;
	drag_context.start_screen_pos = selected.hand_pos;
	drag_context.hand_pos = selected.hand_pos;
	drag_context.hand_slot = selected.hand_slot;
	drag_context.board_anchor = { -1,-1 };
	drag_context.cursor_offset = selected.hand_pos - cursor_pos;
	drag_context.start_rotation = selected.rotation;
	selected.lifecycle = BattleCardRules::CardLifecycle::DraggingFromHand;
	selected.visual_motion = {};
	TraceTransition(U"drag-start-hand", deck_index);
	is_board_active = true; // Boardをアクティブにする
	return true;
}
