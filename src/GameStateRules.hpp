#ifndef GAME_STATE_RULES_HPP
#define GAME_STATE_RULES_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <string_view>
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
		for (int32_t y = 1; y <= 3; ++y) {
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

	void UnlockAll() noexcept {
		m_unlocked.fill(true);
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
	return std::min<int32_t>(15, (progress.UnlockedCount() + 1) / 2 + 2);
}

[[nodiscard]] inline int32_t ResolveBattleHandLimit(const BoardProgress& progress,
	const int32_t explicit_override) noexcept {
	return (0 < explicit_override)
		? std::min<int32_t>(15, explicit_override)
		: CalculateHandLimit(progress);
}

[[nodiscard]] inline std::vector<int32_t> CreateInitialDrawOrder(
	const int32_t card_count, const bool preserve_deck_order) {
	std::vector<int32_t> result(static_cast<std::size_t>(std::max(card_count, 0)));
	std::iota(result.begin(), result.end(), 0);
	if (preserve_deck_order) std::reverse(result.begin(), result.end());
	return result;
}

[[nodiscard]] inline bool ShouldShuffleInitialDrawOrder(
	const bool preserve_deck_order) noexcept {
	return !preserve_deck_order;
}

[[nodiscard]] inline int32_t RequiredBoardFillCells(const int32_t unlocked_cells) noexcept {
	return (std::max(unlocked_cells, 0) + 1) / 2;
}

[[nodiscard]] inline bool IsOccupiedCardSymbol(const char symbol) noexcept {
	return (symbol != '$') && (symbol != '\n') && (symbol != '\r');
}

[[nodiscard]] inline int32_t CountOccupiedCells(const std::string_view card_definition) noexcept {
	return static_cast<int32_t>(std::count_if(card_definition.begin(), card_definition.end(),
		IsOccupiedCardSymbol));
}

[[nodiscard]] inline bool ShouldRefreshDrawPile(const int32_t remaining_occupied_cells,
	const int32_t full_deck_occupied_cells, const int32_t unlocked_cells) noexcept {
	const int32_t required_cells = RequiredBoardFillCells(unlocked_cells);
	return (remaining_occupied_cells < required_cells)
		&& (required_cells <= full_deck_occupied_cells);
}

struct AudioSettings {
	double bgm_volume = 1.0;
	double se_volume = 1.0;
};

[[nodiscard]] inline double ClampVolume(const double volume) noexcept {
	return std::clamp(volume, 0.0, 1.0);
}

[[nodiscard]] inline int32_t VolumePercent(const double volume) noexcept {
	return static_cast<int32_t>(std::lround(ClampVolume(volume) * 100.0));
}

[[nodiscard]] inline double SliderVolumeAt(const double pointer_x, const double track_x,
	const double track_width) noexcept {
	if (track_width <= 0.0) return 0.0;
	return ClampVolume((pointer_x - track_x) / track_width);
}

[[nodiscard]] inline uint64_t ElapsedMillis(const uint64_t now, const uint64_t start) noexcept {
	return (start <= now) ? (now - start) : 0;
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

[[nodiscard]] inline bool NeedsBoardDetach(const CardZone expected,
	const CardZone destination) noexcept {
	const bool was_board_managed = (expected == CardZone::Hand) || (expected == CardZone::Board);
	const bool remains_board_managed = (destination == CardZone::Hand) || (destination == CardZone::Board);
	return was_board_managed && !remains_board_managed;
}

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

	[[nodiscard]] bool CanMove(const int32_t card_id, const CardZone expected,
		const CardZone destination) const noexcept {
		if (!IsValidCard(card_id) || (ZoneOf(card_id) != expected)) return false;
		if (expected == destination) return true;
		const auto& source = Container(expected);
		const auto& target = Container(destination);
		return (std::find(source.begin(), source.end(), card_id) != source.end())
			&& (std::find(target.begin(), target.end(), card_id) == target.end());
	}

	bool Move(const int32_t card_id, const CardZone expected, const CardZone destination) {
		if (!CanMove(card_id, expected, destination)) return false;
		if (expected == destination) return true;
		auto& source = Container(expected);
		const auto it = std::find(source.begin(), source.end(), card_id);
		auto& target = Container(destination);
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

	bool RefreshDrawPile(std::vector<int32_t> draw_order) {
		if (!m_hand.empty() || !m_board.empty() || !Validate() || !IsCompleteOrder(draw_order)) {
			return false;
		}
		m_zones.assign(m_zones.size(), CardZone::DrawPile);
		m_draw_pile = std::move(draw_order);
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
