#ifndef GAME_STATE_RULES_HPP
#define GAME_STATE_RULES_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

namespace GameStateRules {

struct BoardCell {
	int32_t x = -1;
	int32_t y = -1;

	friend constexpr bool operator==(const BoardCell&, const BoardCell&) = default;
};

class BoardProgress {
public:
	static constexpr int32_t Width = 7;
	static constexpr int32_t Height = 6;
	static constexpr int32_t CellCount = Width * Height;

	BoardProgress() {
		Reset();
	}

	void Reset() {
		m_unlocked.fill(false);
		for (int32_t y = 2; y <= 3; ++y) {
			for (int32_t x = 2; x <= 4; ++x) {
				m_unlocked[ToIndex({ x, y })] = true;
			}
		}
	}

	[[nodiscard]] bool IsInside(const BoardCell cell) const noexcept {
		return (0 <= cell.x) && (cell.x < Width) && (0 <= cell.y) && (cell.y < Height);
	}

	[[nodiscard]] bool IsUnlocked(const BoardCell cell) const noexcept {
		return IsInside(cell) && m_unlocked[ToIndex(cell)];
	}

	[[nodiscard]] bool IsUnlockable(const BoardCell cell) const noexcept {
		if (!IsInside(cell) || IsUnlocked(cell)) return false;
		if (cell.y < 2) return IsUnlocked({ cell.x, cell.y + 1 });
		if (cell.y < 4) {
			return IsUnlocked({ cell.x - 1, cell.y }) || IsUnlocked({ cell.x + 1, cell.y });
		}
		return IsUnlocked({ cell.x, cell.y - 1 });
	}

	[[nodiscard]] std::vector<BoardCell> UnlockableCells() const {
		std::vector<BoardCell> result;
		for (int32_t y = 0; y < Height; ++y) {
			for (int32_t x = 0; x < Width; ++x) {
				if (IsUnlockable({ x, y })) result.push_back({ x, y });
			}
		}
		return result;
	}

	bool Unlock(const BoardCell cell) {
		if (!IsUnlockable(cell)) return false;
		m_unlocked[ToIndex(cell)] = true;
		return true;
	}

	[[nodiscard]] int32_t UnlockedCount() const noexcept {
		return static_cast<int32_t>(std::count(m_unlocked.begin(), m_unlocked.end(), true));
	}

private:
	[[nodiscard]] static constexpr std::size_t ToIndex(const BoardCell cell) noexcept {
		return static_cast<std::size_t>(cell.y * Width + cell.x);
	}

	std::array<bool, CellCount> m_unlocked{};
};

[[nodiscard]] inline int32_t CalculateHandLimit(const BoardProgress& progress) noexcept {
	return std::min<int32_t>(15, progress.UnlockedCount() / 2 + 2);
}

enum class RunOutcome {
	None,
	Clear,
	GameOver,
};

[[nodiscard]] inline int32_t ActIndex(const int32_t layer) noexcept {
	return std::clamp(layer / 10, 0, 2);
}

[[nodiscard]] inline int32_t FloorInAct(const int32_t layer) noexcept {
	return std::max(layer, 0) % 10;
}

enum class VictoryDestination {
	Map,
	NextActBattle,
	Result,
};

struct VictoryProgress {
	VictoryDestination destination = VictoryDestination::Map;
	int32_t next_layer = 0;
};

[[nodiscard]] inline VictoryProgress ResolveVictory(const int32_t enemy_type,
	const int32_t layer) noexcept {
	if ((enemy_type == 2) && (layer >= 29)) {
		return { VictoryDestination::Result, layer };
	}
	if ((enemy_type == 2) && ((layer == 9) || (layer == 19))) {
		return { VictoryDestination::NextActBattle, layer + 1 };
	}
	return { VictoryDestination::Map, layer };
}

enum class CardZone {
	DrawPile,
	Hand,
	Board,
	Discard,
};

struct CardZoneChange {
	int32_t card_id = -1;
	CardZone expected = CardZone::DrawPile;
	CardZone destination = CardZone::DrawPile;
};

class BattleDeckState {
public:
	void Initialize(const int32_t card_count, std::vector<int32_t> draw_order = {}) {
		m_zones.assign(static_cast<std::size_t>(std::max(card_count, 0)), CardZone::DrawPile);
		m_draw_pile.clear();
		m_hand.clear();
		m_board.clear();
		m_discard.clear();
		if (draw_order.empty()) {
			draw_order.resize(m_zones.size());
			std::iota(draw_order.begin(), draw_order.end(), 0);
		}
		if (!IsCompleteOrder(draw_order)) return;
		m_draw_pile = std::move(draw_order);
	}

	[[nodiscard]] int32_t CardCount() const noexcept {
		return static_cast<int32_t>(m_zones.size());
	}

	[[nodiscard]] bool IsValidCard(const int32_t card_id) const noexcept {
		return (0 <= card_id) && (card_id < CardCount());
	}

	[[nodiscard]] CardZone ZoneOf(const int32_t card_id) const noexcept {
		return IsValidCard(card_id) ? m_zones[static_cast<std::size_t>(card_id)] : CardZone::DrawPile;
	}

	[[nodiscard]] const std::vector<int32_t>& Cards(const CardZone zone) const noexcept {
		return Container(zone);
	}

	bool Move(const int32_t card_id, const CardZone expected, const CardZone destination) {
		if (!IsValidCard(card_id) || (ZoneOf(card_id) != expected)) return false;
		if (expected == destination) return true;
		auto& source = Container(expected);
		const auto it = std::find(source.begin(), source.end(), card_id);
		if (it == source.end()) return false;
		auto& target = Container(destination);
		if (std::find(target.begin(), target.end(), card_id) != target.end()) return false;
		source.erase(it);
		target.push_back(card_id);
		m_zones[static_cast<std::size_t>(card_id)] = destination;
		return true;
	}

	bool RecycleDiscard() {
		if (!m_draw_pile.empty() || !m_hand.empty() || !m_board.empty() || m_discard.empty()) return false;
		for (const int32_t card_id : m_discard) {
			m_zones[static_cast<std::size_t>(card_id)] = CardZone::DrawPile;
		}
		m_draw_pile = m_discard;
		m_discard.clear();
		return true;
	}

	[[nodiscard]] bool Validate() const noexcept {
		std::vector<int32_t> counts(m_zones.size(), 0);
		for (const CardZone zone : { CardZone::DrawPile, CardZone::Hand, CardZone::Board, CardZone::Discard }) {
			for (const int32_t card_id : Container(zone)) {
				if (!IsValidCard(card_id) || (ZoneOf(card_id) != zone)) return false;
				counts[static_cast<std::size_t>(card_id)]++;
			}
		}
		return std::all_of(counts.begin(), counts.end(), [](const int32_t count) { return count == 1; });
	}

private:
	[[nodiscard]] bool IsCompleteOrder(const std::vector<int32_t>& order) const {
		if (order.size() != m_zones.size()) return false;
		std::vector<bool> seen(m_zones.size(), false);
		for (const int32_t card_id : order) {
			if (!IsValidCard(card_id) || seen[static_cast<std::size_t>(card_id)]) return false;
			seen[static_cast<std::size_t>(card_id)] = true;
		}
		return true;
	}

	[[nodiscard]] const std::vector<int32_t>& Container(const CardZone zone) const noexcept {
		switch (zone) {
		case CardZone::DrawPile: return m_draw_pile;
		case CardZone::Hand: return m_hand;
		case CardZone::Board: return m_board;
		case CardZone::Discard: return m_discard;
		}
		return m_draw_pile;
	}

	[[nodiscard]] std::vector<int32_t>& Container(const CardZone zone) noexcept {
		return const_cast<std::vector<int32_t>&>(std::as_const(*this).Container(zone));
	}

	std::vector<CardZone> m_zones;
	std::vector<int32_t> m_draw_pile;
	std::vector<int32_t> m_hand;
	std::vector<int32_t> m_board;
	std::vector<int32_t> m_discard;
};

}

#endif
