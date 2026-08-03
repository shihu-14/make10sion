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
    if (IsBoardBlockIndexValid(resolved_index)
        && (board_blocks[resolved_index].block == drag_context.block)) return resolved_index;
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
        && (ResolveDragBlockIndex() == drag_context.board_block_index);
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

bool Board::BlocksOverlap(int32 index_1, Point anchor_1, int32 index_2, Point anchor_2) const {
    if (!IsBoardBlockIndexValid(index_1) || !IsBoardBlockIndexValid(index_2)) return true;
    Array<Point> cells_1;
    Array<Point> cells_2;
    if (!GetBlockCells(*board_blocks[index_1].block, anchor_1, cells_1)
        || !GetBlockCells(*board_blocks[index_2].block, anchor_2, cells_2)) return true;
    for (const auto& cell : cells_1) {
        if (cells_2.includes(cell)) return true;
    }
    return false;
}

void Board::CaptureBoardBlockSnapshots() {
    drag_context.board_block_snapshots.clear();
#ifndef NDEBUG
    drag_context.board_block_snapshots.reserve(board_blocks.size());
    for (const auto& state : board_blocks) {
        BoardBlockSnapshot snapshot;
        snapshot.block = state.block;
        snapshot.deck_index = state.deck_index;
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
        for (int32 j = 0; j < i; j++) {
            if ((board_blocks[i].deck_index == board_blocks[j].deck_index)
                || (board_blocks[i].block == board_blocks[j].block)) return false;
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
    const Point drop_cell = ScreenToBoardCell(Cursor::Pos());
    if (drop_cell != Point{ -1,-1 }) {
        const int32 usage = board_usage[drop_cell.y][drop_cell.x];
        if (usage > 0) {
            const int32 target_index = usage - 1;
            if (target_index != selected_index) {
                if (!IsBoardBlockPlaced(target_index)) return plan;
                const Point target_anchor = board_blocks[target_index].board_anchor;
                if (!drag_context.from_board) {
                    if (CanPlaceBlock(selected_index, target_anchor, selected_index, target_index)) {
                        plan.type = DropType::HandBoardSwap;
                        plan.anchor = target_anchor;
                        plan.target_block_index = target_index;
                    }
                    return plan;
                }

                const Point selected_anchor = drag_context.board_anchor;
                if (CanPlaceBlock(selected_index, target_anchor, selected_index, target_index)
                    && CanPlaceBlock(target_index, selected_anchor, selected_index, target_index)
                    && !BlocksOverlap(selected_index, target_anchor, target_index, selected_anchor)) {
                    plan.type = DropType::BoardBoardSwap;
                    plan.anchor = target_anchor;
                    plan.target_block_index = target_index;
                }
                return plan;
            }
        }
    }

    if (candidate_anchor == Point{ -1,-1 }) return plan;
    if (CanPlaceBlock(selected_index, candidate_anchor, selected_index)) {
        plan.type = DropType::Place;
        plan.anchor = candidate_anchor;
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

bool Board::RestoreDrag() {
    if (!drag_context.active || (drag_context.block == nullptr)) return false;
    for (int32 i = 0; i < (4 - drag_context.rotation_count) % 4; i++) {
        drag_context.block->Rotate();
    }
    drag_context.block->SetPos(drag_context.start_screen_pos.x, drag_context.start_screen_pos.y);
    drag_context.block->SetStat(drag_context.start_stat);

    const int32 selected_index = ResolveDragBlockIndex();
    if (!IsBoardBlockIndexValid(selected_index)) return false;
    BoardBlockState& selected = board_blocks[selected_index];
    selected.rotation = drag_context.start_rotation;
    selected.hand_pos = drag_context.hand_pos;
    if (drag_context.from_board) {
        selected.board_anchor = drag_context.board_anchor;
        selected.animation = 0;
    } else {
        selected.board_anchor = { -1,-1 };
        selected.animation = -1;
    }
    return true;
}

void Board::ClearDrag() {
    drag_context = {};
}

void Board::PutBlock() {//blockがドロップされたら、配置/交換/元の場所への復元を行う
    if (!IsDragContextValid()) {
        const bool restored = RestoreDrag();
        assert(restored);
        CalcRow();
        AssertBoardState();
        ClearDrag();
        return;
    }
    const DropPlan plan = AnalyzeDrop(PutBlockAt());
    const int32 selected_index = drag_context.board_block_index;
    BoardBlockState& selected = board_blocks[selected_index];
    if (plan.type == DropType::Invalid) {
        const bool restored = RestoreDrag();
        assert(restored);
        CalcRow();
        AssertBoardState();
        ClearDrag();
        return;
    }

    if (plan.type == DropType::Place) {
        if (drag_context.from_board) ClearBoardBlock(selected_index);
        UpdateBoardNum(selected_index, plan.anchor);
        SetBoardBlockPosition(selected_index, plan.anchor);
        selected.animation = 0;
        selected.block->SetStat(2);
    } else if (plan.type == DropType::HandBoardSwap) {
        const int32 target_index = plan.target_block_index;
        BoardBlockState& target = board_blocks[target_index];
        ClearBoardBlock(target_index);
        UpdateBoardNum(selected_index, plan.anchor);
        SetBoardBlockPosition(selected_index, plan.anchor);
        selected.animation = 0;
        selected.block->SetStat(2);
        SetBlockRotation(target_index, 0);
        target.hand_pos = drag_context.hand_pos;
        target.block->SetPos(target.hand_pos.x, target.hand_pos.y);
        target.animation = -1;
        target.block->SetStat(1);
    } else if (plan.type == DropType::BoardBoardSwap) {
        const int32 target_index = plan.target_block_index;
        const Point selected_anchor = drag_context.board_anchor;
        BoardBlockState& target = board_blocks[target_index];
        ClearBoardBlock(selected_index);
        ClearBoardBlock(target_index);
        UpdateBoardNum(selected_index, plan.anchor);
        UpdateBoardNum(target_index, selected_anchor);
        SetBoardBlockPosition(selected_index, plan.anchor);
        SetBoardBlockPosition(target_index, selected_anchor);
        selected.animation = 0;
        target.animation = 0;
        selected.block->SetStat(2);
        target.block->SetStat(2);
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
    drag_context.start_stat = selected.block->GetStat();
    drag_context.start_screen_pos = { selected.block->GetPos().first, selected.block->GetPos().second };
    drag_context.hand_pos = selected.hand_pos;
    drag_context.board_anchor = selected.board_anchor;
    drag_context.start_rotation = selected.rotation;
    drag_context.rotation_count = 0;
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
    const BoardBlockState& selected = board_blocks[selected_index];
    drag_context.active = true;
    drag_context.board_block_index = selected_index;
    drag_context.deck_index = deck_index;
    drag_context.block = &selectedBlock;
    drag_context.from_board = false;
    drag_context.start_stat = selectedBlock.GetStat();
    drag_context.start_screen_pos = hand_pos;
    drag_context.hand_pos = hand_pos;
    drag_context.board_anchor = { -1,-1 };
    drag_context.start_rotation = selected.rotation;
    drag_context.rotation_count = 0;
    CaptureBoardBlockSnapshots();
    is_board_active = true; // Boardをアクティブにする
    return true;
}
