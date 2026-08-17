#ifndef BOARD_CALCULATION_RULES_HPP
#define BOARD_CALCULATION_RULES_HPP

#include "CardSymbolRules.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace BoardCalculationRules {

struct Cell {
	char symbol = '\0';
	int32_t front_bonus = 0;
	bool occupied = false;
};

class Board {
public:
	Board(const int32_t width, const int32_t height)
		: m_width(std::max(width, 0)), m_height(std::max(height, 0)),
		m_cells(static_cast<std::size_t>(m_width * m_height)) {}

	void Set(const int32_t x, const int32_t y, const char symbol, const int32_t front_bonus = 0) {
		if (!IsInside(x, y)) return;
		m_cells[ToIndex(x, y)] = { symbol, front_bonus, true };
	}

	[[nodiscard]] int32_t Width() const noexcept { return m_width; }
	[[nodiscard]] int32_t Height() const noexcept { return m_height; }

	[[nodiscard]] const Cell& At(const int32_t x, const int32_t y) const noexcept {
		static constexpr Cell Empty{};
		return IsInside(x, y) ? m_cells[ToIndex(x, y)] : Empty;
	}

private:
	[[nodiscard]] bool IsInside(const int32_t x, const int32_t y) const noexcept {
		return (0 <= x) && (x < m_width) && (0 <= y) && (y < m_height);
	}

	[[nodiscard]] std::size_t ToIndex(const int32_t x, const int32_t y) const noexcept {
		return static_cast<std::size_t>(y * m_width + x);
	}

	int32_t m_width = 0;
	int32_t m_height = 0;
	std::vector<Cell> m_cells;
};

struct Evaluation {
	std::vector<int32_t> row_values;
	std::vector<bool> row_valid;
	std::vector<double> row_multiplier_effects;
	std::vector<int32_t> row_modes;
	std::vector<int32_t> numbers;
};

template <class Storage>
inline void CommitDelayedEffects(const Storage& current, Storage& committed) {
	committed = current;
}

template <class Storage>
inline void AdvanceDelayedEffects(Storage& active, Storage& current, Storage& committed) {
	active = committed;
	current.fill(0);
	committed.fill(0);
}

namespace Detail {

struct Token {
	bool is_number = false;
	double number = 0.0;
	CardSymbolRules::Operator operation = CardSymbolRules::Operator::None;
};

[[nodiscard]] inline std::optional<int32_t> CheckedInt(const double value) noexcept {
	if (!std::isfinite(value)
		|| (value < static_cast<double>(std::numeric_limits<int32_t>::min()))
		|| (static_cast<double>(std::numeric_limits<int32_t>::max()) < value)) return std::nullopt;
	return static_cast<int32_t>(value);
}

[[nodiscard]] inline bool IsSafeIntermediate(const double value) noexcept {
	return std::isfinite(value)
		&& (static_cast<double>(std::numeric_limits<int32_t>::min()) <= value)
		&& (value <= static_cast<double>(std::numeric_limits<int32_t>::max()));
}

[[nodiscard]] inline std::optional<int32_t> EvaluateTokens(const std::vector<Token>& tokens) noexcept {
	if (tokens.empty()) return int32_t{ 0 };
	if (!tokens.front().is_number || !tokens.back().is_number || ((tokens.size() % 2) == 0)) {
		return std::nullopt;
	}
	for (std::size_t i = 0; i < tokens.size(); ++i) {
		if (tokens[i].is_number != ((i % 2) == 0)) return std::nullopt;
	}

	double total = 0.0;
	double term = tokens.front().number;
	if (!IsSafeIntermediate(term)) return std::nullopt;
	CardSymbolRules::Operator pending_add = CardSymbolRules::Operator::Add;
	for (std::size_t i = 1; i < tokens.size(); i += 2) {
		const auto operation = tokens[i].operation;
		const double value = tokens[i + 1].number;
		if (!IsSafeIntermediate(value)) return std::nullopt;
		if (operation == CardSymbolRules::Operator::Multiply) {
			term *= value;
		} else if (operation == CardSymbolRules::Operator::Divide) {
			if (value == 0.0) return std::nullopt;
			term /= value;
		} else if ((operation == CardSymbolRules::Operator::Add)
			|| (operation == CardSymbolRules::Operator::Subtract)) {
			total += (pending_add == CardSymbolRules::Operator::Subtract) ? -term : term;
			term = value;
			pending_add = operation;
		} else {
			return std::nullopt;
		}
		if (!IsSafeIntermediate(term) || !IsSafeIntermediate(total)) return std::nullopt;
	}
	total += (pending_add == CardSymbolRules::Operator::Subtract) ? -term : term;
	if (!IsSafeIntermediate(total)) return std::nullopt;
	return CheckedInt(total);
}

} // namespace Detail

[[nodiscard]] inline Evaluation Evaluate(const Board& board, const int32_t attack_row_count = 3) {
	Evaluation result;
	result.row_values.assign(static_cast<std::size_t>(board.Height()), 0);
	result.row_valid.assign(static_cast<std::size_t>(board.Height()), true);
	result.row_multiplier_effects.assign(static_cast<std::size_t>(board.Height()), 0.0);
	result.row_modes.assign(static_cast<std::size_t>(board.Height()), 0);
	for (int32_t y = 0; y < std::min(attack_row_count, board.Height()); ++y) result.row_modes[y] = 1;

	for (int32_t y = 0; y < board.Height(); ++y) {
		for (int32_t x = 0; x < board.Width(); ++x) {
			const Cell& cell = board.At(x, y);
			if (!cell.occupied) continue;
			const auto definition = CardSymbolRules::Decode(cell.symbol);
			if (definition.kind == CardSymbolRules::Kind::Number) {
				const int64_t value = static_cast<int64_t>(definition.current_value) + cell.front_bonus;
				if ((std::numeric_limits<int32_t>::min() <= value)
					&& (value <= std::numeric_limits<int32_t>::max())) {
					result.numbers.push_back(static_cast<int32_t>(value));
				}
			} else if (definition.kind == CardSymbolRules::Kind::RowMultiplier) {
				result.row_multiplier_effects[y] = definition.row_multiplier;
			} else if (definition.kind == CardSymbolRules::Kind::RowMode) {
				result.row_modes[y] = (definition.row_mode == CardSymbolRules::RowMode::Attack) ? 1 : 0;
			}
		}
	}
	std::sort(result.numbers.begin(), result.numbers.end());

	for (int32_t y = 0; y < board.Height(); ++y) {
		std::vector<Detail::Token> tokens;
		bool before_was_number = false;
		bool before_was_operator = false;
		for (int32_t x = 0; x < board.Width(); ++x) {
			const Cell& cell = board.At(x, y);
			if (!cell.occupied) continue;
			const auto definition = CardSymbolRules::Decode(cell.symbol);
			if (definition.kind == CardSymbolRules::Kind::Number) {
				if (before_was_number) continue;
				tokens.push_back({ true,
					static_cast<double>(definition.current_value) + cell.front_bonus });
				before_was_number = true;
				before_was_operator = false;
			} else if (definition.kind == CardSymbolRules::Kind::Operator) {
				if (before_was_operator || !before_was_number) continue;
				tokens.push_back({ false, 0.0, definition.operation });
				before_was_number = false;
				before_was_operator = true;
			} else if (definition.kind == CardSymbolRules::Kind::Aggregate) {
				if (before_was_number || result.numbers.empty()) continue;
				double value = 0.0;
				if (definition.aggregate == CardSymbolRules::Aggregate::Maximum) {
					value = result.numbers.back();
				} else if (definition.aggregate == CardSymbolRules::Aggregate::Minimum) {
					value = result.numbers.front();
				} else {
					for (const int32_t number : result.numbers) value += number;
					value /= static_cast<double>(result.numbers.size());
				}
				tokens.push_back({ true, value });
				before_was_number = true;
			}
		}
		if (!tokens.empty() && !tokens.back().is_number) tokens.pop_back();
		const auto value = Detail::EvaluateTokens(tokens);
		if (value) result.row_values[y] = *value;
		else result.row_valid[y] = false;
	}
	return result;
}

[[nodiscard]] inline std::optional<int32_t> CheckedRowContribution(
	const int32_t value, const double multiplier) noexcept {
	return Detail::CheckedInt(static_cast<double>(value) * multiplier);
}

[[nodiscard]] inline bool CheckedAdd(int32_t& total, const int32_t contribution) noexcept {
	const int64_t value = static_cast<int64_t>(total) + contribution;
	if ((value < std::numeric_limits<int32_t>::min())
		|| (std::numeric_limits<int32_t>::max() < value)) return false;
	total = static_cast<int32_t>(value);
	return true;
}

} // namespace BoardCalculationRules

#endif
