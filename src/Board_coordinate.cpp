#include <Siv3D.hpp>
#include "common.hpp"
#include "Board.hpp"
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
    return drag_context.active
        && (drag_context.block != nullptr)
        && IsBoardBlockIndexValid(drag_context.board_block_index)
        && (board_blocks[drag_context.board_block_index].deck_index == drag_context.deck_index)
        && (board_blocks[drag_context.board_block_index].block == drag_context.block);
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
    double min_dist = 10000.0;
    Point anchor = { -1,-1 };
    for (int32 y = 0; y < static_cast<int32>(board_usage.height()); y++) {
        for (int32 x = 0; x < static_cast<int32>(board_usage.width()); x++) {
            const Point cell = { x,y };
            const double distance = CalcDist(GetBoardCellCenter(cell), first_piece_pos);
            if (distance < min_dist) {
                min_dist = distance;
                anchor = cell;
            }
        }
    }
    return anchor;
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
        if (board_usage[cell.y][cell.x] != index + 1) return false;
    }
    size_t occupied_count = 0;
    for (const auto& usage : board_usage) {
        if (usage == index + 1) occupied_count++;
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
        const int32 occupied_index = usage - 1;
        if ((occupied_index != ignored_index_1) && (occupied_index != ignored_index_2)) return false;
    }
    return true;
}

void Board::CaptureBoardBlockSnapshots() {
    drag_context.board_block_snapshots.clear();
#ifndef NDEBUG
    drag_context.board_block_snapshots.reserve(board_blocks.size());
    for (const auto& state : board_blocks) {
        BoardBlockSnapshot snapshot;
        snapshot.block = state.block;
        snapshot.deck_index = state.deck_index;
        snapshot.hand_pos = state.hand_pos;
        snapshot.board_anchor = state.board_anchor;
        snapshot.rotation = state.rotation;
        snapshot.animation = state.animation;
        if (state.block) {
            snapshot.screen_pos = { state.block->GetPos().first, state.block->GetPos().second };
            snapshot.stat = state.block->GetStat();
        }
        drag_context.board_block_snapshots.push_back(snapshot);
    }
#endif
}

bool Board::ValidateBoardState(int32 allowed_target_index) const {
    for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
        if (!IsBoardBlockIndexValid(i)) return false;
        if (board_blocks[i].hand_pos == Point{ -1,-1 }) return false;
        for (int32 j = 0; j < i; j++) {
            if ((board_blocks[i].deck_index == board_blocks[j].deck_index)
                || (board_blocks[i].block == board_blocks[j].block)
                || (board_blocks[i].hand_pos == board_blocks[j].hand_pos)) return false;
        }
    }

    for (const auto& usage : board_usage) {
        if (usage <= 0) continue;
        const int32 index = usage - 1;
        if (!IsBoardBlockIndexValid(index) || (board_blocks[index].block->GetStat() != 2)) return false;
    }

    for (int32 index = 0; index < static_cast<int32>(board_blocks.size()); index++) {
        const BoardBlockState& state = board_blocks[index];
        size_t occupied_count = 0;
        for (const auto& usage : board_usage) {
            if (usage == index + 1) occupied_count++;
        }

        if (state.block->GetStat() == 1) {
            if ((state.board_anchor != Point{ -1,-1 }) || (state.animation != -1) || (occupied_count != 0)) return false;
            continue;
        }
        if (state.block->GetStat() != 2) {
            if (occupied_count != 0) return false;
            continue;
        }
        Array<Point> expected_cells;
        if ((state.animation != 0)
            || !GetBlockCells(*state.block, state.board_anchor, expected_cells)
            || (occupied_count != expected_cells.size())
            || !IsBoardBlockPlaced(index)) return false;
        const Point screen_pos = { state.block->GetPos().first, state.block->GetPos().second };
        if (screen_pos != GetBoardBlockScreenPosition(*state.block, state.board_anchor)) return false;
        for (int32 y = 0; y < state.block->Size().second; y++) {
            for (int32 x = 0; x < state.block->Size().first; x++) {
                const Piece& piece = state.block->GetPiece(x, y);
                if (piece.content == '$') continue;
                const Point piece_screen_pos = screen_pos + GetScaledPieceOffset(piece);
                if (piece_screen_pos != GetBoardCellCenter(state.board_anchor + Point{ x,y })) return false;
            }
        }
    }

#ifndef NDEBUG
    int32 allowed_target_deck_index = -1;
    if (IsBoardBlockIndexValid(allowed_target_index)) {
        allowed_target_deck_index = board_blocks[allowed_target_index].deck_index;
    }
    for (const auto& snapshot : drag_context.board_block_snapshots) {
        if ((snapshot.deck_index == drag_context.deck_index)
            || (snapshot.deck_index == allowed_target_deck_index)) continue;
        const int32 index = FindBoardBlockIndex(snapshot.deck_index);
        if (!IsBoardBlockIndexValid(index)) return false;
        const BoardBlockState& state = board_blocks[index];
        const Point screen_pos = { state.block->GetPos().first, state.block->GetPos().second };
        if ((state.block != snapshot.block)
            || (screen_pos != snapshot.screen_pos)
            || (state.hand_pos != snapshot.hand_pos)
            || (state.board_anchor != snapshot.board_anchor)
            || (state.rotation != snapshot.rotation)
            || (state.block->GetStat() != snapshot.stat)
            || (state.animation != snapshot.animation)) return false;
    }
#else
    (void)allowed_target_index;
#endif
    return true;
}

void Board::AssertBoardState(int32 allowed_target_index) const {
#ifndef NDEBUG
    assert(ValidateBoardState(allowed_target_index));
#else
    (void)allowed_target_index;
#endif
}

Point Board::PutBlockAt() const {//blockの置ける場所を確認. blockの(0, 0)のピースのボード座標を返す
    if (!IsDragContextValid()) return { -1,-1 };
    const Block& block = *board_blocks[drag_context.board_block_index].block;
    const Point screen_pos = { block.GetPos().first, block.GetPos().second };
    return GetBoardAnchorFromScreenPosition(block, screen_pos);
}

Board::DropPlan Board::AnalyzeDrop(Point candidate_anchor) const {
    DropPlan plan;
    if (!IsDragContextValid()) return plan;
    const int32 selected_index = drag_context.board_block_index;
    if ((ScreenToBoardCell(Cursor::Pos()) == Point{ -1,-1 })
        || (candidate_anchor == Point{ -1,-1 })) return plan;

    Array<Point> candidate_cells;
    if (!GetBlockCells(*board_blocks[selected_index].block, candidate_anchor, candidate_cells)) return plan;

    Array<int32> overlapping_indices;
    bool overlaps_original_cells = false;
    bool has_unusable_cell = false;
    for (const auto& cell : candidate_cells) {
        const int32 usage = board_usage[cell.y][cell.x];
        if (usage < 0) {
            has_unusable_cell = true;
            continue;
        }
        if (usage <= 0) continue;
        const int32 occupied_index = usage - 1;
        if (occupied_index == selected_index) {
            overlaps_original_cells = true;
        } else if (!overlapping_indices.includes(occupied_index)) {
            overlapping_indices.push_back(occupied_index);
        }
    }

    if (drag_context.from_board) {
        const Point screen_pos = {
            board_blocks[selected_index].block->GetPos().first,
            board_blocks[selected_index].block->GetPos().second
        };
        const double restore_distance = static_cast<double>(cell_size * cell_size) / 4.0;
        if (overlaps_original_cells
            || !overlapping_indices.isEmpty()
            || (candidate_anchor == drag_context.board_anchor)
            || (CalcDist(screen_pos, drag_context.start_screen_pos) <= restore_distance)) {
            plan.type = DropType::RestoreToBoard;
            return plan;
        }
        if (has_unusable_cell) return plan;
        if (CanPlaceBlock(selected_index, candidate_anchor, selected_index)) {
            plan.type = DropType::Place;
            plan.anchor = candidate_anchor;
        }
        return plan;
    }

    if (has_unusable_cell) return plan;
    if (overlapping_indices.isEmpty()) {
        if (CanPlaceBlock(selected_index, candidate_anchor, selected_index)) {
            plan.type = DropType::Place;
            plan.anchor = candidate_anchor;
        }
        return plan;
    }
    if (overlapping_indices.size() != 1) return plan;

    const int32 target_index = overlapping_indices.front();
    if (!IsBoardBlockPlaced(target_index)) return plan;
    const BoardBlockState& target = board_blocks[target_index];
    if ((target.hand_pos == Point{ -1,-1 })
        || (drag_context.hand_pos == Point{ -1,-1 })
        || (target.hand_pos == drag_context.hand_pos)) return plan;
    if (CanPlaceBlock(selected_index, target.board_anchor, selected_index, target_index)) {
        plan.type = DropType::Swap;
        plan.anchor = target.board_anchor;
        plan.target_block_index = target_index;
    }
    return plan;
}

void Board::ClearBoardBlock(int32 index) {
    if (!IsBoardBlockIndexValid(index)) return;
    for (size_t y = 0; y < board_usage.height(); y++) {
        for (size_t x = 0; x < board_usage.width(); x++) {
            if (board_usage[y][x] != index + 1) continue;
            board_usage[y][x] = 0;
            board_number[y][x] = 0;
            board_effect_back[y][x] = 0;
            board_content[y][x] = '\0';
        }
    }
    board_blocks[index].board_anchor = { -1,-1 };
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

bool Board::ReturnDraggedBlockToHand() {
    if (!drag_context.active) return false;
    const int32 selected_index = ResolveDragBlockIndex();
    const Point return_pos = drag_context.from_board
        ? drag_context.hand_pos
        : drag_context.start_screen_pos;
    if (IsBoardBlockIndexValid(selected_index)) {
        BoardBlockState& selected = board_blocks[selected_index];
        ClearBoardBlock(selected_index);
        SetBlockRotation(selected_index, 0);
        selected.hand_pos = return_pos;
        selected.block->SetPos(return_pos.x, return_pos.y);
        selected.board_anchor = { -1,-1 };
        selected.block->SetStat(1);
        selected.animation = -1;
    } else if (drag_context.block != nullptr) {
        const int32 usage_value = drag_context.board_block_index + 1;
        if ((0 < usage_value) && !IsBoardBlockIndexValid(drag_context.board_block_index)) {
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
        int32 rotation = (drag_context.start_rotation + drag_context.rotation_steps) % 4;
        while (rotation != 0) {
            drag_context.block->Rotate();
            rotation = (rotation + 1) % 4;
        }
        drag_context.block->SetPos(return_pos.x, return_pos.y);
        drag_context.block->SetStat(1);
    } else {
        ClearDrag();
        return false;
    }
    CalcRow();
    if (IsBoardBlockIndexValid(selected_index)) AssertBoardState();
    ClearDrag();
    return true;
}

bool Board::RestoreDraggedBlockToBoard() {
    if (!drag_context.active || !drag_context.from_board) return false;
    const int32 selected_index = ResolveDragBlockIndex();
    if (!IsBoardBlockIndexValid(selected_index)) {
        ReturnDraggedBlockToHand();
        return false;
    }

    ClearBoardBlock(selected_index);
    SetBlockRotation(selected_index, drag_context.start_rotation);
    if (!CanPlaceBlock(selected_index, drag_context.board_anchor, selected_index)) {
        ReturnDraggedBlockToHand();
        return false;
    }
    BoardBlockState& selected = board_blocks[selected_index];
    UpdateBoardNum(selected_index, drag_context.board_anchor);
    SetBoardBlockPosition(selected_index, drag_context.board_anchor);
    selected.animation = 0;
    selected.block->SetStat(2);
    CalcRow();
    AssertBoardState();
    ClearDrag();
    return true;
}

void Board::ClearDrag() {
    drag_context = {};
}

void Board::PutBlock() {//blockがドロップされたら、配置/交換/手札への復帰を行う
    if (!IsDragContextValid()) {
        if (drag_context.from_board) RestoreDraggedBlockToBoard();
        else ReturnDraggedBlockToHand();
        return;
    }
    const DropPlan plan = AnalyzeDrop(PutBlockAt());
    const int32 selected_index = drag_context.board_block_index;
    BoardBlockState& selected = board_blocks[selected_index];
    if (plan.type == DropType::ReturnToHand) {
        ReturnDraggedBlockToHand();
        return;
    }
    if (plan.type == DropType::RestoreToBoard) {
        RestoreDraggedBlockToBoard();
        return;
    }

    if (plan.type == DropType::Place) {
        if (drag_context.from_board) ClearBoardBlock(selected_index);
        UpdateBoardNum(selected_index, plan.anchor);
        SetBoardBlockPosition(selected_index, plan.anchor);
        selected.animation = 0;
        selected.block->SetStat(2);
    } else if ((plan.type == DropType::Swap) && !drag_context.from_board) {
        const int32 target_index = plan.target_block_index;
        BoardBlockState& target = board_blocks[target_index];
        const Point selected_hand_pos = drag_context.hand_pos;
        const Point target_hand_pos = target.hand_pos;
        ClearBoardBlock(target_index);
        UpdateBoardNum(selected_index, plan.anchor);
        SetBoardBlockPosition(selected_index, plan.anchor);
        selected.hand_pos = target_hand_pos;
        selected.animation = 0;
        selected.block->SetStat(2);
        SetBlockRotation(target_index, 0);
        target.hand_pos = selected_hand_pos;
        target.block->SetPos(target.hand_pos.x, target.hand_pos.y);
        target.animation = -1;
        target.block->SetStat(1);
    }
    CalcRow();
    AssertBoardState(plan.target_block_index);
    ClearDrag();
}

void Board::TakeOutBlock(Point pos) {//クリックしたBlockのドラッグを開始する
    const int32 board_width = static_cast<int32>(board_usage.width());
    const int32 board_height = static_cast<int32>(board_usage.height());
    if (drag_context.active
        || (pos.x < 0) || (board_width <= pos.x)
        || (pos.y < 0) || (board_height <= pos.y)) return;
    const int32 usage = board_usage[pos.y][pos.x];
    if (usage <= 0) return;
    AssertBoardState();
    const int32 selected_index = usage - 1;
    if (!IsBoardBlockPlaced(selected_index)) return;
    BoardBlockState& selected = board_blocks[selected_index];
    if (selected.block->GetStat() != 2) return;
    drag_context.active = true;
    drag_context.board_block_index = selected_index;
    drag_context.deck_index = selected.deck_index;
    drag_context.block = selected.block;
    drag_context.from_board = true;
    drag_context.start_screen_pos = { selected.block->GetPos().first, selected.block->GetPos().second };
    drag_context.hand_pos = selected.hand_pos;
    drag_context.board_anchor = selected.board_anchor;
    drag_context.cursor_offset = drag_context.start_screen_pos - Cursor::Pos();
    drag_context.start_rotation = selected.rotation;
    selected.animation = 0;
    CaptureBoardBlockSnapshots();
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
bool Board::PassBlock(Block& selectedBlock, int32 deck_index, const Point hand_pos) {//選択されているBlockとそのDeck番号、手札座標が渡される
    if (drag_context.active || (deck_index < 0) || (selectedBlock.GetStat() != 1)) return false;
    for (const auto& state : board_blocks) {
        if ((state.deck_index != deck_index) && (state.hand_pos == hand_pos)) return false;
    }
    AssertBoardState();
    int32 selected_index = FindBoardBlockIndex(deck_index);
    if (selected_index < 0) {//新出のブロックなら
        for (const auto& state : board_blocks) {
            if (state.block == &selectedBlock) return false;
        }
        BoardBlockState state;
        state.block = &selectedBlock;
        state.deck_index = deck_index;
        state.hand_pos = hand_pos;
        state.animation = 0;
        board_blocks.push_back(state);
        selected_index = static_cast<int32>(board_blocks.size()) - 1;
    } else {//既出のブロックなら
        if (!IsBoardBlockIndexValid(selected_index)) return false;
        BoardBlockState& state = board_blocks[selected_index];
        if (state.board_anchor != Point{ -1,-1 }) return false;
        state.block = &selectedBlock;
        state.hand_pos = hand_pos;
        state.animation = 0;
    }
    drag_context.active = true;
    drag_context.board_block_index = selected_index;
    drag_context.deck_index = deck_index;
    drag_context.block = &selectedBlock;
    drag_context.from_board = false;
    drag_context.start_screen_pos = hand_pos;
    drag_context.hand_pos = hand_pos;
    drag_context.board_anchor = { -1,-1 };
    drag_context.cursor_offset = hand_pos - Cursor::Pos();
    drag_context.start_rotation = board_blocks[selected_index].rotation;
    CaptureBoardBlockSnapshots();
    is_board_active = true; // Boardをアクティブにする
    return true;
}
