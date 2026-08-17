#include "Block.hpp"
#include "Board.hpp"
#include "BoardCalculationRules.hpp"
#include "CardSymbolRules.hpp"
#include <Siv3D.hpp>
#include <array>
#include <limits>

void Board::RebuildBoardDerivedState() {
	board_multiply_effect.fill(0);
	board_off_def.fill(0);
	const int32 board_width = Min(static_cast<int32>(board_usage.width()), static_cast<int32>(board_content.width()));
	const int32 grid_height = Min(static_cast<int32>(board_usage.height()), static_cast<int32>(board_content.height()));
	const int32 state_height = Min(static_cast<int32>(board_multiply_effect.size()), static_cast<int32>(board_off_def.size()));
	const int32 board_height = Min(grid_height, state_height);
	const int32 attack_row_count = Min(Max(off_count, 0), board_height);
	for (int y = 0; y < attack_row_count; y++) {
		board_off_def[y] = 1;
	}

	for (int y = 0; y < board_height; y++) {
		for (int x = 0; x < board_width; x++) {
			if (board_usage[y][x] <= 0) continue;
			const auto definition = CardSymbolRules::Decode(board_content[y][x]);
			if (definition.kind == CardSymbolRules::Kind::RowMultiplier) {
				board_multiply_effect[y] = definition.row_multiplier;
			} else if (definition.kind == CardSymbolRules::Kind::RowMode) {
				board_off_def[y] = (definition.row_mode == CardSymbolRules::RowMode::Attack) ? 1 : 0;
			}
		}
	}
}

void Board::CalcRow() {
	const int32 board_width = static_cast<int32>(board_usage.width());
	const int32 board_height = static_cast<int32>(board_usage.height());
	BoardCalculationRules::Board calculation_board{ board_width, board_height };
	for (int32 y = 0; y < board_height; ++y) {
		for (int32 x = 0; x < board_width; ++x) {
			if (board_usage[y][x] <= 0) continue;
			calculation_board.Set(x, y, board_content[y][x], board_effect_front[y][x]);
		}
	}
	const auto evaluation = BoardCalculationRules::Evaluate(calculation_board, off_count);
	num_on_board.assign(evaluation.numbers.begin(), evaluation.numbers.end());
	result_of_calc.assign(evaluation.row_values.begin(), evaluation.row_values.end());
	row_valid.assign(evaluation.row_valid.begin(), evaluation.row_valid.end());
	board_multiply_effect.assign(evaluation.row_multiplier_effects.begin(), evaluation.row_multiplier_effects.end());
	board_off_def.assign(evaluation.row_modes.begin(), evaluation.row_modes.end());
}



std::pair<int, int> Board::Confirm() {
	CalcRow();
	int32 attack = 0, defense = 0;
	const int32 row_count = Min({ static_cast<int32>(result_of_calc.size()),
		static_cast<int32>(board_multiply.size()), static_cast<int32>(board_multiply_effect.size()),
		static_cast<int32>(board_off_def.size()) });
	for (int32 i = 0; i < row_count; i++) {
		if ((i < static_cast<int32>(row_valid.size())) && !row_valid[i]) {
			Logger << U"Invalid board expression treated as zero: row=" << i;
		}
		const auto contribution = BoardCalculationRules::CheckedRowContribution(
			result_of_calc[i], board_multiply[i] + board_multiply_effect[i]);
		if (!contribution) {
			Logger << U"Ignored out-of-range board row contribution: row=" << i;
			continue;
		}
		int32& total = (board_off_def[i] == 1) ? attack : defense;
		if (!BoardCalculationRules::CheckedAdd(total, *contribution)) {
			Logger << U"Ignored overflowing board row contribution: row=" << i;
		}
	}
	int32 placed_block_count = 0;
	for (int32 i = 0; i < static_cast<int32>(board_blocks.size()); i++) {
		if (IsBoardBlockPlaced(i)) placed_block_count++;
	}
	const int64 attack_bonus = static_cast<int64>(add_damage)
		+ static_cast<int64>(add_damage_by_cards) * placed_block_count;
	if ((std::numeric_limits<int32>::min() <= attack_bonus)
		&& (attack_bonus <= std::numeric_limits<int32>::max())) {
		if (!BoardCalculationRules::CheckedAdd(attack, static_cast<int32>(attack_bonus))) {
			Logger << U"Ignored overflowing attack bonus";
		}
	} else {
		Logger << U"Ignored out-of-range attack bonus";
	}
	if (!BoardCalculationRules::CheckedAdd(defense, add_armor)) {
		Logger << U"Ignored overflowing armor bonus";
	}
	if (do_armor_raise) {
		if (defense < 6)defense = 6;
	}
	return { attack, defense };
}


void Board::Discard() {
	board_number.fill(0);
	board_content.fill('\0');
	num_on_board.clear();
	result_of_calc.fill(0);
	row_valid.fill(true);
	BoardCalculationRules::AdvanceDelayedEffects(
		board_effect_front, board_effect_back, board_effect_committed);
	RebuildBoardDerivedState();
}



bool Board::UpdateBoardNum(int32 index, Point putAt) {
	if (!IsBoardBlockIndexValid(index)) return false;
	Block* block = board_blocks[index].block;
	Array<Point> cells;
	if (!GetBlockCells(*block, putAt, cells)) return false;
	for (int i = 0; i < block->Size().second; i++) {
		for (int j = 0; j < block->Size().first; j++) {
			char content = block->GetPiece(j, i).content;
			if (content == '$')continue; // $は無視
			board_usage[putAt.y + i][putAt.x + j] = board_blocks[index].deck_index + 1;
			GetPieceNum(content, putAt.y + i, putAt.x + j); // 数字の取得&マスの変更
		}
	}
	return true;
}



void Board::GetPieceNum(char content, int y, int x) {
	board_number[y][x] = 0;
	board_effect_back[y][x] = 0;
	board_content[y][x] = content;
	const auto definition = CardSymbolRules::Decode(content);
	if (definition.kind == CardSymbolRules::Kind::Number) {
		board_number[y][x] = definition.current_value;
		board_effect_back[y][x] = definition.next_turn_bonus;
	} else if (definition.kind == CardSymbolRules::Kind::Unknown) {
		static std::array<bool, 256> reported{};
		const auto index = static_cast<unsigned char>(content);
		if (!reported[index]) {
			reported[index] = true;
			Logger << U"Unknown card symbol ignored: " << static_cast<int32>(index);
		}
	}
}
