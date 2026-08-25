#ifndef BATTLE_LAYOUT_RULES_HPP
#define BATTLE_LAYOUT_RULES_HPP

#include <cstdint>

namespace BattleLayoutRules {

struct ScreenPoint {
	int32_t x = 0;
	int32_t y = 0;

	friend constexpr bool operator==(const ScreenPoint&, const ScreenPoint&) = default;
};

struct BoardCell {
	int32_t x = -1;
	int32_t y = -1;

	friend constexpr bool operator==(const BoardCell&, const BoardCell&) = default;
};

struct ScreenRect {
	int32_t x = 0;
	int32_t y = 0;
	int32_t width = 0;
	int32_t height = 0;

	[[nodiscard]] constexpr bool Contains(const ScreenRect other) const noexcept {
		return (x <= other.x) && (y <= other.y)
			&& (other.x + other.width <= x + width)
			&& (other.y + other.height <= y + height);
	}

	[[nodiscard]] constexpr bool Intersects(const ScreenRect other) const noexcept {
		return (x < other.x + other.width) && (other.x < x + width)
			&& (y < other.y + other.height) && (other.y < y + height);
	}
};

inline constexpr int32_t SceneWidth = 1920;
inline constexpr int32_t SceneHeight = 1080;
inline constexpr int32_t BoardWidth = 7;
inline constexpr int32_t BoardHeight = 6;
inline constexpr int32_t BoardCellSize = 90;

[[nodiscard]] inline constexpr ScreenRect SceneBounds() noexcept { return { 0, 0, SceneWidth, SceneHeight }; }
[[nodiscard]] inline constexpr ScreenPoint BoardOffset() noexcept { return { 480, 190 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerPosition() noexcept { return { 90, 230 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerHpPosition() noexcept { return { 20, 700 }; }
[[nodiscard]] inline constexpr ScreenPoint EnemyPosition() noexcept { return { 1750, 500}; }
[[nodiscard]] inline constexpr ScreenPoint EnemyHpPosition() noexcept { return { 1570, 700 }; }
[[nodiscard]] inline constexpr ScreenPoint DrawPilePosition() noexcept { return { 50, 800 }; }
[[nodiscard]] inline constexpr ScreenPoint DiscardPilePosition() noexcept { return { 1720, 880 }; }
[[nodiscard]] inline constexpr ScreenPoint DiscardTarget() noexcept { return { 1730, 950 }; }
[[nodiscard]] inline constexpr ScreenRect DiscardPileBounds() noexcept { return { 1720, 880, 180, 180 }; }
[[nodiscard]] inline constexpr ScreenRect EqualButtonBounds() noexcept { return { 1725, 750, 175, 105 }; }
[[nodiscard]] inline constexpr ScreenPoint HandPosition(const int32_t slot) noexcept {
	return { 350 + slot * 75, 900 };
}
[[nodiscard]] inline constexpr ScreenRect HandCardBounds(const int32_t slot) noexcept {
	const auto position = HandPosition(slot);
	return { position.x - 25, position.y - 25, 50, 50 };
}
[[nodiscard]] inline constexpr ScreenPoint BoardCellCenter(const BoardCell cell) noexcept {
	return { BoardOffset().x + cell.x * BoardCellSize + BoardCellSize / 2,
		BoardOffset().y + cell.y * BoardCellSize + BoardCellSize / 2 };
}
[[nodiscard]] inline constexpr BoardCell BoardCellAt(const ScreenPoint point) noexcept {
	const int32_t relative_x = point.x - BoardOffset().x;
	const int32_t relative_y = point.y - BoardOffset().y;
	if ((relative_x < 0) || (relative_y < 0)
		|| (BoardWidth * BoardCellSize <= relative_x)
		|| (BoardHeight * BoardCellSize <= relative_y)) return {};
	return { relative_x / BoardCellSize, relative_y / BoardCellSize };
}

inline constexpr int32_t BoardRight = BoardOffset().x + BoardWidth * BoardCellSize;
inline constexpr int32_t ResultColumnRight = BoardRight + 145;
inline constexpr int32_t MultiplierColumnLeft = BoardRight + 170;
inline constexpr int32_t PlayerCombatIconX = 350;
inline constexpr int32_t PlayerCombatValueX = 430;
inline constexpr int32_t EnemyCombatIconX = 1390;
inline constexpr int32_t EnemyCombatValueX = 1470;
inline constexpr double PlayerDisplayScale = 0.85;
inline constexpr int32_t EnemyHitOffsetX = -250;
inline constexpr int32_t EnemyHitOffsetY = -100;
inline constexpr int32_t EnemyDamageEffectMinOffsetX = -430;
inline constexpr int32_t EnemyDamageEffectMinOffsetY = -300;
inline constexpr int32_t EnemyDamageEffectWidth = 390;
inline constexpr int32_t EnemyDamageEffectHeight = 250;
[[nodiscard]] inline constexpr ScreenPoint PlayerAttackStart() noexcept { return { PlayerCombatIconX, 640 }; }
[[nodiscard]] inline constexpr ScreenPoint EnemyDefenseTarget() noexcept { return { EnemyCombatIconX, 760 }; }
[[nodiscard]] inline constexpr ScreenPoint EnemyHitTarget() noexcept {
	return { EnemyPosition().x + EnemyHitOffsetX, EnemyPosition().y + EnemyHitOffsetY };
}
[[nodiscard]] inline constexpr ScreenRect EnemyDamageEffectBounds() noexcept {
	return { EnemyPosition().x + EnemyDamageEffectMinOffsetX,
		EnemyPosition().y + EnemyDamageEffectMinOffsetY,
		EnemyDamageEffectWidth, EnemyDamageEffectHeight };
}
[[nodiscard]] inline constexpr ScreenPoint EnemyAttackStart() noexcept { return { EnemyCombatIconX, 640 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerDefenseTarget() noexcept { return { PlayerCombatIconX, 760 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerHitTarget() noexcept { return { 80, 250 }; }

inline constexpr double EnemyDisplayHeight = 340.0;
inline constexpr double EnemyLegacyBaseScale = 0.85;
inline constexpr double EnemyHitScaleMultiplier = 0.7 / EnemyLegacyBaseScale;
inline constexpr double EnemyScaleRecoveryRate = 1.0 / EnemyLegacyBaseScale;

[[nodiscard]] inline constexpr double EnemyBaseScale(const int32_t texture_height) noexcept {
	return (0 < texture_height) ? (EnemyDisplayHeight / texture_height) : 1.0;
}

[[nodiscard]] inline constexpr double EnemyDisplayScale(
	const int32_t texture_height, const double animation_multiplier) noexcept {
	return EnemyBaseScale(texture_height) * animation_multiplier;
}

} // namespace BattleLayoutRules

#endif
