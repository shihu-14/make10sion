#ifndef CARD_SYMBOL_RULES_HPP
#define CARD_SYMBOL_RULES_HPP

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace CardSymbolRules {

enum class Kind {
	Unknown,
	Hole,
	Number,
	Operator,
	OccupiedOnly,
	Aggregate,
	RowMultiplier,
	RowMode,
};

enum class Operator {
	None,
	Add,
	Subtract,
	Multiply,
	Divide,
};

enum class Aggregate {
	None,
	Maximum,
	Minimum,
	Average,
};

enum class RowMode {
	None,
	Attack,
	Defense,
};

struct Definition {
	Kind kind = Kind::Unknown;
	int32_t current_value = 0;
	int32_t next_turn_bonus = 0;
	double row_multiplier = 0.0;
	Operator operation = Operator::None;
	Aggregate aggregate = Aggregate::None;
	RowMode row_mode = RowMode::None;
	int32_t visual_number = -1;

	[[nodiscard]] constexpr bool IsKnown() const noexcept {
		return kind != Kind::Unknown;
	}
};

[[nodiscard]] constexpr Definition Decode(const char symbol) noexcept {
	if (('0' <= symbol) && (symbol <= '7')) {
		return { Kind::Number, symbol - '0', 0, 0.0, Operator::None,
			Aggregate::None, RowMode::None, symbol - '0' };
	}
	if (('A' <= symbol) && (symbol <= 'H')) {
		return { Kind::OccupiedOnly, 0, 0, 0.0, Operator::None,
			Aggregate::None, RowMode::None, symbol - 'A' };
	}
	switch (symbol) {
	case '$': return { Kind::Hole };
	case '+': return { Kind::Operator, 0, 0, 0.0, Operator::Add };
	case '-': return { Kind::Operator, 0, 0, 0.0, Operator::Subtract };
	case '*': return { Kind::Operator, 0, 0, 0.0, Operator::Multiply };
	case '/': return { Kind::Operator, 0, 0, 0.0, Operator::Divide };
	case 'a': return { Kind::RowMultiplier, 0, 0, 1.0 };
	case 'b': return { Kind::RowMultiplier, 0, 0, 1.5 };
	case 'c': return { Kind::RowMultiplier, 0, 0, 2.0 };
	case 'd': return { Kind::OccupiedOnly };
	case 'e': return { Kind::Aggregate, 0, 0, 0.0, Operator::None, Aggregate::Average };
	case 'f': return { Kind::RowMode, 0, 0, 0.0, Operator::None, Aggregate::None, RowMode::Defense };
	case 'g': return { Kind::Aggregate, 0, 0, 0.0, Operator::None, Aggregate::Maximum };
	case 'h': return { Kind::Aggregate, 0, 0, 0.0, Operator::None, Aggregate::Minimum };
	case 'i': return { Kind::RowMode, 0, 0, 0.0, Operator::None, Aggregate::None, RowMode::Attack };
	case 'j': return { Kind::Number, 1, 2 };
	case 'k': return { Kind::Number, 2, 2 };
	case 'l': return { Kind::Number, 2, 4 };
	case 'm': return { Kind::Number, 12 };
	case 'n': return { Kind::OccupiedOnly };
	case 'o': return { Kind::Number, 2, 1 };
	case 'p': return { Kind::Number, 3, 1 };
	case 'q': return { Kind::Number, 1, 1 };
	default: return {};
	}
}

[[nodiscard]] constexpr bool IsValidCardDefinition(const std::string_view definition) noexcept {
	if (definition.empty()) return false;
	std::size_t expected_width = 0;
	std::size_t current_width = 0;
	for (const char symbol : definition) {
		if (symbol == '\n') {
			if ((current_width == 0) || ((expected_width != 0) && (current_width != expected_width))) return false;
			if (expected_width == 0) expected_width = current_width;
			current_width = 0;
			continue;
		}
		if (!Decode(symbol).IsKnown()) return false;
		++current_width;
	}
	return (current_width != 0) && ((expected_width == 0) || (current_width == expected_width));
}

} // namespace CardSymbolRules

#endif
