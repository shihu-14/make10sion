#ifndef BATTLE_LAYOUT_RULES_HPP
#define BATTLE_LAYOUT_RULES_HPP

#include <cstdint>
#include <Siv3D.hpp>

namespace BattleLayoutRules {

struct CombatAttackMotion {
	s3d::Vec2 position;
	double scale = 1.0;
};

inline constexpr int32_t BoardWidth = 7;
inline constexpr int32_t BoardHeight = 6;
inline constexpr int32_t BoardCellSize = 90;
inline constexpr int32_t HandCardCellSize = 50;
inline constexpr int32_t HandCardGap = 25;
inline constexpr int32_t HandLeft = 325;
inline constexpr int32_t HandY = 900;
inline constexpr double HandHoverScale = 1.3;
inline constexpr int32_t HandHoverLift = 15;

inline const s3d::Point BoardOffset{ 480, 190 };
inline const s3d::Point PlayerPosition{ 70, 270 };
inline const s3d::Point PlayerHpPosition{ 25, 650 };
inline const s3d::Point EnemyPosition{ 1750, 450 };
inline const s3d::Point EnemyHpPosition{ 1570, 650 };
inline const s3d::Point DrawPilePosition{ 50, 800 };
inline const s3d::Point DiscardPilePosition{ 1720, 880 };
inline const s3d::Point DiscardTarget{ 1730, 950 };
inline const s3d::Rect EqualButtonBounds{ 1725, 750, 175, 105 };
inline s3d::Point HandPosition(int32_t slot,
	int32_t widthBefore, int32_t cardWidth) {
	return {
		HandLeft + widthBefore * HandCardCellSize + slot * HandCardGap
			+ cardWidth * HandCardCellSize / 2,
		HandY,
	};
}
inline s3d::Point BoardCellCenter(s3d::Point cell) {
	return { BoardOffset.x + cell.x * BoardCellSize + BoardCellSize / 2,
		BoardOffset.y + cell.y * BoardCellSize + BoardCellSize / 2 };
}
inline s3d::Point BoardCellAt(s3d::Point point) {
	const int32_t relative_x = point.x - BoardOffset.x;
	const int32_t relative_y = point.y - BoardOffset.y;
	if ((relative_x < 0) || (relative_y < 0)
		|| (BoardWidth * BoardCellSize <= relative_x)
		|| (BoardHeight * BoardCellSize <= relative_y)) return { -1, -1 };
	return { relative_x / BoardCellSize, relative_y / BoardCellSize };
}

inline constexpr int32_t BoardRight = 480 + BoardWidth * BoardCellSize;
inline constexpr int32_t ResultColumnRight = BoardRight + 145;
inline constexpr int32_t MultiplierColumnLeft = BoardRight + 170;
inline const s3d::Point PlayerAttackIcon{ 270, 500 };
inline const s3d::Point PlayerDefenseIcon{ 270, 590 };
inline const s3d::Point PlayerAttackValue{ 360, 500 };
inline const s3d::Point PlayerDefenseValue{ 360, 590 };
inline const s3d::Point EnemyAttackIcon{ 1400, 500 };
inline const s3d::Point EnemyDefenseIcon{ 1400, 590 };
inline const s3d::Point EnemyAttackValue{ 1490, 500 };
inline const s3d::Point EnemyDefenseValue{ 1490, 590 };
inline const s3d::Point CombatValueAnimationOffset{ 80, 0 };
inline constexpr double AttackIconScale = 1.6;
inline constexpr double DefenseIconScale = 1.5;
inline constexpr int32_t CombatFontSize = 48;
inline constexpr double CombatStatPulseDuration = 0.35;
inline constexpr double CombatStatPulsePeakScale = 1.08;
inline constexpr double CombatStatPulseLift = 8.0;
inline constexpr double AttackArcHeight = 200.0;
inline constexpr double AttackArcArrivalScale = 1.4;
inline constexpr double RewardFadeInDuration = 0.5;
inline constexpr int32_t RewardBlurFilterSize = 13;
inline constexpr int32_t RewardBlurPassCount = 2;
inline constexpr double PlayerDisplayScale = 0.6;
inline constexpr int32_t EnemyHitOffsetX = -250;
inline constexpr int32_t EnemyHitOffsetY = -100;
inline const s3d::Point EnemyDamageEffectRandomOffset{ 150, 150 };
inline const s3d::Point PlayerDamageEffectCenter{ 125, 180 };
inline const s3d::Point PlayerDamageEffectRandomOffset{ 75, 75 };

struct CombatStatPulse {
	double scale = 1.0;
	double lift = 0.0;
};

inline CombatStatPulse ResolveCombatStatPulse(
	double elapsed_seconds, bool active) {
	if (!active || (elapsed_seconds < 0.0)
		|| (CombatStatPulseDuration <= elapsed_seconds)) return {};
	const double half_duration = CombatStatPulseDuration / 2.0;
	const double linear_peak = (elapsed_seconds <= half_duration)
		? (elapsed_seconds / half_duration)
		: ((CombatStatPulseDuration - elapsed_seconds) / half_duration);
	const double eased_peak = linear_peak * linear_peak * (3.0 - 2.0 * linear_peak);
	return {
		1.0 + (CombatStatPulsePeakScale - 1.0) * eased_peak,
		CombatStatPulseLift * eased_peak,
	};
}

inline const s3d::Point EnemyAttackArcTarget{ 1400, 250 };
inline const s3d::Point PlayerAttackArcTarget{ 250, 250 };

inline const s3d::Point PlayerAttackStart = PlayerAttackIcon;
inline CombatAttackMotion ResolveAttackArcMotion(
	s3d::Point start, s3d::Point end,
	double progress) {
	const double inverse = 1.0 - progress;
	const double control_x = (start.x + end.x) / 2.0;
	const double control_y = (start.y + end.y) / 2.0 - AttackArcHeight;
	return {
		{ inverse * inverse * start.x + 2.0 * inverse * progress * control_x + progress * progress * end.x,
		  inverse * inverse * start.y + 2.0 * inverse * progress * control_y + progress * progress * end.y },
		1.0 + (AttackArcArrivalScale - 1.0) * progress,
	};
}
inline const s3d::Point EnemyHitTarget{ EnemyPosition.x + EnemyHitOffsetX, EnemyPosition.y + EnemyHitOffsetY };
inline const s3d::Rect EnemyDamageEffectBounds{ EnemyPosition.x - EnemyDamageEffectRandomOffset.x,
	EnemyPosition.y - EnemyDamageEffectRandomOffset.y,
	EnemyDamageEffectRandomOffset.x * 2, EnemyDamageEffectRandomOffset.y * 2 };
inline const s3d::Rect PlayerDamageEffectBounds{ PlayerDamageEffectCenter.x - PlayerDamageEffectRandomOffset.x,
	PlayerDamageEffectCenter.y - PlayerDamageEffectRandomOffset.y,
	PlayerDamageEffectRandomOffset.x * 2, PlayerDamageEffectRandomOffset.y * 2 };
inline const s3d::Point EnemyAttackStart = EnemyAttackIcon;
inline const s3d::Point PlayerHitTarget{ 80, 350 };

inline constexpr double EnemyDisplayHeight = 340.0;
inline constexpr double EnemyLegacyBaseScale = 0.85;
inline constexpr double EnemyHitScaleMultiplier = 0.7 / EnemyLegacyBaseScale;
inline constexpr double EnemyScaleRecoveryRate = 1.0 / EnemyLegacyBaseScale;

inline double EnemyBaseScale(int32_t texture_height) {
	return (0 < texture_height) ? (EnemyDisplayHeight / texture_height) : 1.0;
}

}

#endif
