#ifndef BATTLE_LAYOUT_RULES_HPP
#define BATTLE_LAYOUT_RULES_HPP

#include <cstdint>

namespace BattleLayoutRules {

struct ScreenPoint {
	int32_t x = 0;
	int32_t y = 0;

	friend constexpr bool operator==(const ScreenPoint&, const ScreenPoint&) = default;
};

struct CombatAttackMotion {
	double x = 0.0;
	double y = 0.0;
	double scale = 1.0;
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

inline constexpr int32_t SceneWidth = 1920; // バトル画面の幅を定義する．
inline constexpr int32_t SceneHeight = 1080; // バトル画面の高さを定義する．
inline constexpr int32_t BoardWidth = 7; // 盤面の横マス数を定義する．
inline constexpr int32_t BoardHeight = 6; // 盤面の縦マス数を定義する．
inline constexpr int32_t BoardCellSize = 90; // 盤面1マスの画面サイズを定義する．
inline constexpr int32_t HandCardCellSize = 50; // 手札カード1マスの画面サイズを定義する．
inline constexpr int32_t HandCardGap = 25; // 手札カード同士の端から端までの間隔を定義する．
inline constexpr int32_t HandLeft = 325; // 手札全体の左端座標を定義する．
inline constexpr int32_t HandY = 900; // 手札カード中心のY座標を定義する．
inline constexpr double HandHoverScale = 1.3; // ホバー中の手札カード表示倍率を定義する．
inline constexpr int32_t HandHoverLift = 15; // ホバー中の手札カードを上へ移動する量を定義する．

[[nodiscard]] inline constexpr ScreenRect SceneBounds() noexcept { return { 0, 0, SceneWidth, SceneHeight }; }
[[nodiscard]] inline constexpr ScreenPoint BoardOffset() noexcept { return { 480, 190 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerPosition() noexcept { return { 70, 270 }; }
[[nodiscard]] inline constexpr ScreenPoint PlayerHpPosition() noexcept { return { 25, 650 }; }
[[nodiscard]] inline constexpr ScreenPoint EnemyPosition() noexcept { return { 1750, 450}; }
[[nodiscard]] inline constexpr ScreenPoint EnemyHpPosition() noexcept { return { 1570, 650 }; }
[[nodiscard]] inline constexpr ScreenPoint DrawPilePosition() noexcept { return { 50, 800 }; }
[[nodiscard]] inline constexpr ScreenPoint DiscardPilePosition() noexcept { return { 1720, 880 }; }
[[nodiscard]] inline constexpr ScreenPoint DiscardTarget() noexcept { return { 1730, 950 }; }
[[nodiscard]] inline constexpr ScreenRect DiscardPileBounds() noexcept { return { 1720, 880, 180, 180 }; }
[[nodiscard]] inline constexpr ScreenRect EqualButtonBounds() noexcept { return { 1725, 750, 175, 105 }; }
[[nodiscard]] inline constexpr ScreenPoint HandPosition(const int32_t slot,
	const int32_t preceding_width_cells, const int32_t card_width_cells) noexcept {
	return {
		HandLeft + preceding_width_cells * HandCardCellSize + slot * HandCardGap
			+ card_width_cells * HandCardCellSize / 2,
		HandY,
	};
}
[[nodiscard]] inline constexpr ScreenRect HandCardBounds(const int32_t slot,
	const int32_t preceding_width_cells, const int32_t card_width_cells) noexcept {
	const auto position = HandPosition(slot, preceding_width_cells, card_width_cells);
	const int32_t width = card_width_cells * HandCardCellSize;
	return { position.x - width / 2, position.y - HandCardCellSize / 2,
		width, HandCardCellSize };
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

inline constexpr int32_t BoardRight = BoardOffset().x + BoardWidth * BoardCellSize; // 盤面の右端座標を定義する．
inline constexpr int32_t ResultColumnRight = BoardRight + 145; // 行結果表示の右端を定義する．
inline constexpr int32_t MultiplierColumnLeft = BoardRight + 170; // 倍率表示の左端を定義する．
inline constexpr ScreenPoint PlayerCombatAttackIconPosition{ 270, 500 }; // プレイヤー攻撃アイコンの位置を定義する．
inline constexpr ScreenPoint PlayerCombatDefenseIconPosition{ 270, 590 }; // プレイヤー防御アイコンの位置を定義する．
inline constexpr ScreenPoint PlayerCombatAttackValuePosition{ 360, 500 }; // プレイヤー攻撃値の位置を定義する．
inline constexpr ScreenPoint PlayerCombatDefenseValuePosition{ 360, 590 }; // プレイヤー防御値の位置を定義する．
inline constexpr ScreenPoint EnemyCombatAttackIconPosition{ 1400, 500 }; // 敵攻撃アイコンの位置を定義する．
inline constexpr ScreenPoint EnemyCombatDefenseIconPosition{ 1400, 590 }; // 敵防御アイコンの位置を定義する．
inline constexpr ScreenPoint EnemyCombatAttackValuePosition{ 1490, 500 }; // 敵攻撃値の位置を定義する．
inline constexpr ScreenPoint EnemyCombatDefenseValuePosition{ 1490, 590 }; // 敵防御値の位置を定義する．
inline constexpr ScreenPoint CombatValueAnimationOffset{ 80, 0 }; // 演出中の数値位置補正を定義する．
inline constexpr double AttackIconScale = 1.6; // 攻撃アイコンの表示倍率を定義する．
inline constexpr double DefenseIconScale = 1.5; // 防御アイコンの表示倍率を定義する．
inline constexpr int32_t CombatFontSize = 48; // 攻撃・防御値のフォントサイズを定義する．
inline constexpr double CombatStatPulseDuration = 0.35; // 攻撃・防御値の強調時間を定義する．
inline constexpr double CombatStatPulsePeakScale = 1.08; // 攻撃・防御値の最大表示倍率を定義する．
inline constexpr double CombatStatPulseLift = 8.0; // 強調時に上へ移動する最大量を定義する．
inline constexpr double AttackArcTravelDuration = 0.5; // 攻撃アイコンが弧を描く時間を定義する．
inline constexpr double AttackArcHeight = 200.0; // 攻撃軌道の上方向への膨らみを定義する．
inline constexpr double AttackArcArrivalScale = 1.4; // 攻撃アイコンと数値の到着時倍率を定義する．
inline constexpr double BodyAttackTravelDuration = 0.5; // 防御突破後に本体へ移動する時間を定義する．
inline constexpr double DamageEffectInterval = 0.3; // 本体への連続攻撃エフェクト間隔を定義する．
inline constexpr double PlayerDisplayScale = 0.6; // プレイヤー画像の表示倍率を定義する．
inline constexpr int32_t EnemyHitOffsetX = -250; // 敵被弾位置のX補正値を定義する．
inline constexpr int32_t EnemyHitOffsetY = -100; // 敵被弾位置のY補正値を定義する．
inline constexpr int32_t EnemyDamageEffectMinOffsetX = -430; // 敵ダメージ演出の左端補正値を定義する．
inline constexpr int32_t EnemyDamageEffectMinOffsetY = -300; // 敵ダメージ演出の上端補正値を定義する．
inline constexpr int32_t EnemyDamageEffectWidth = 390; // 敵ダメージ演出の幅を定義する．
inline constexpr int32_t EnemyDamageEffectHeight = 250; // 敵ダメージ演出の高さを定義する．

struct PlayerCombatValueVisibility {
	bool attack = false;
	bool defense = false;
};

struct CombatStatPulse {
	double scale = 1.0;
	double lift = 0.0;
};

struct PlayerCombatStatPulseChanges {
	bool attack = false;
	bool defense = false;
};

[[nodiscard]] inline constexpr PlayerCombatStatPulseChanges
	ResolvePlayerCombatStatPulseChanges(
		const int32_t previous_attack, const int32_t previous_defense,
		const int32_t current_attack, const int32_t current_defense) noexcept {
	return {
		previous_attack != current_attack,
		previous_defense != current_defense,
	};
}

[[nodiscard]] inline constexpr CombatStatPulse ResolveCombatStatPulse(
	const double elapsed_seconds, const bool value_changed) noexcept {
	if (!value_changed || (elapsed_seconds < 0.0)
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

[[nodiscard]] inline constexpr PlayerCombatValueVisibility ResolvePlayerCombatValueVisibility(
	const bool is_idle, const bool is_combat_enemy_effect,
	const bool is_combat_my_effect, const bool is_discard_effect) noexcept {
	return { is_idle,
		is_idle || is_combat_enemy_effect || is_combat_my_effect || is_discard_effect };
}

inline constexpr ScreenPoint EnemyAttackArcTargetPosition{ 1400, 250 }; // 敵の左上にある攻撃軌道の到着位置を定義する．
inline constexpr ScreenPoint PlayerAttackArcTargetPosition{ 250, 250 }; // プレイヤーの右上にある攻撃軌道の到着位置を定義する．

[[nodiscard]] inline constexpr ScreenPoint PlayerAttackStart() noexcept {
	return PlayerCombatAttackIconPosition;
}
[[nodiscard]] inline constexpr ScreenPoint EnemyAttackArcTarget() noexcept {
	return EnemyAttackArcTargetPosition;
}
[[nodiscard]] inline constexpr ScreenPoint PlayerAttackArcTarget() noexcept {
	return PlayerAttackArcTargetPosition;
}
[[nodiscard]] inline constexpr CombatAttackMotion ResolveAttackArcMotion(
	const ScreenPoint start, const ScreenPoint end,
	const double elapsed_seconds) noexcept {
	const double progress = (elapsed_seconds <= 0.0) ? 0.0
		: ((AttackArcTravelDuration <= elapsed_seconds) ? 1.0
			: elapsed_seconds / AttackArcTravelDuration);
	const double inverse = 1.0 - progress;
	const double control_x = (start.x + end.x) / 2.0;
	const double control_y = (start.y + end.y) / 2.0 - AttackArcHeight;
	return {
		inverse * inverse * start.x + 2.0 * inverse * progress * control_x
			+ progress * progress * end.x,
		inverse * inverse * start.y + 2.0 * inverse * progress * control_y
			+ progress * progress * end.y,
		1.0 + (AttackArcArrivalScale - 1.0) * progress,
	};
}
[[nodiscard]] inline constexpr int32_t ResolveCombatValueChange(
	const int32_t start, const int32_t end, const double elapsed_seconds,
	const double duration_seconds) noexcept {
	if (elapsed_seconds <= 0.0) return start;
	if ((duration_seconds <= 0.0) || (duration_seconds <= elapsed_seconds)) return end;
	const double progress = elapsed_seconds / duration_seconds;
	return static_cast<int32_t>(start + (end - start) * progress);
}
[[nodiscard]] inline constexpr double ResolveBodyAttackAlpha(
	const double elapsed_seconds) noexcept {
	if (elapsed_seconds <= 0.0) return 1.0;
	if (BodyAttackTravelDuration <= elapsed_seconds) return 0.0;
	return 1.0 - elapsed_seconds / BodyAttackTravelDuration;
}
[[nodiscard]] inline constexpr double DamageEffectTriggerTime(
	const int32_t hit_index) noexcept {
	return (hit_index <= 0) ? 0.0 : DamageEffectInterval * hit_index;
}
[[nodiscard]] inline constexpr bool ShouldTriggerDamageEffect(
	const double elapsed_seconds, const int32_t hit_index) noexcept {
	return DamageEffectTriggerTime(hit_index) <= elapsed_seconds;
}
[[nodiscard]] inline constexpr ScreenPoint EnemyHitTarget() noexcept {
	return { EnemyPosition().x + EnemyHitOffsetX, EnemyPosition().y + EnemyHitOffsetY };
}
[[nodiscard]] inline constexpr ScreenRect EnemyDamageEffectBounds() noexcept {
	return { EnemyPosition().x + EnemyDamageEffectMinOffsetX,
		EnemyPosition().y + EnemyDamageEffectMinOffsetY,
		EnemyDamageEffectWidth, EnemyDamageEffectHeight };
}
[[nodiscard]] inline constexpr ScreenPoint EnemyAttackStart() noexcept {
	return EnemyCombatAttackIconPosition;
}
[[nodiscard]] inline constexpr ScreenPoint PlayerHitTarget() noexcept { return { 80, 350 }; }

inline constexpr double EnemyDisplayHeight = 340.0; // 敵画像の基準表示高さを定義する．
inline constexpr double EnemyLegacyBaseScale = 0.85; // 旧敵画像の基準倍率を定義する．
inline constexpr double EnemyHitScaleMultiplier = 0.7 / EnemyLegacyBaseScale; // 被弾時の敵画像倍率を定義する．
inline constexpr double EnemyScaleRecoveryRate = 1.0 / EnemyLegacyBaseScale; // 敵画像倍率の復帰率を定義する．

[[nodiscard]] inline constexpr double EnemyBaseScale(const int32_t texture_height) noexcept {
	return (0 < texture_height) ? (EnemyDisplayHeight / texture_height) : 1.0;
}

[[nodiscard]] inline constexpr double EnemyDisplayScale(
	const int32_t texture_height, const double animation_multiplier) noexcept {
	return EnemyBaseScale(texture_height) * animation_multiplier;
}

} // namespace BattleLayoutRules

#endif
