#ifndef BattleDamageRules_HPP
#define BattleDamageRules_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

namespace BattleDamageRules {

inline constexpr std::int32_t MaximumDamageHitCount = 6;
inline constexpr double MaximumDefenseExchangeDuration = 1.0;

struct DefenseExchange {
	std::int32_t attack_after = 0;
	std::int32_t defense_after = 0;
	bool has_contact = false;
};

[[nodiscard]] inline constexpr DefenseExchange ResolveDefenseExchange(
	const std::int32_t attack, const std::int32_t defense,
	const std::int32_t actual_damage) noexcept {
	return {
		std::max(0, actual_damage),
		std::max(0, defense - std::max(0, attack)),
		(0 < attack) && (0 < defense),
	};
}

// ダメージをHP割合に応じた複数ヒットへ変換する．
[[nodiscard]] inline std::int32_t HitCountForDamage(
	const std::int32_t actual_damage, const std::int32_t maximum_hp) noexcept {
	if ((actual_damage <= 0) || (maximum_hp <= 0)) return 0;
	const std::int64_t scaled_damage = static_cast<std::int64_t>(actual_damage) * 100;
	const std::int64_t scaled_maximum = maximum_hp;
	std::int32_t count = MaximumDamageHitCount;
	if (scaled_damage <= 15 * scaled_maximum) count = 1; // 最大HPの15％以下は1回とする．
	else if (scaled_damage <= 30 * scaled_maximum) count = 2; // 最大HPの30％以下は2回とする．
	else if (scaled_damage <= 45 * scaled_maximum) count = 3; // 最大HPの45％以下は3回とする．
	else if (scaled_damage <= 60 * scaled_maximum) count = 4; // 最大HPの60％以下は4回とする．
	else if (scaled_damage <= 75 * scaled_maximum) count = 5; // 最大HPの75％以下は5回とする．
	return std::min(count, actual_damage);
}

[[nodiscard]] inline constexpr double ResolveDefenseExchangeDuration(
	const std::int32_t attack, const std::int32_t defense,
	const std::int32_t maximum_hp) noexcept {
	const std::int32_t blocked_damage = std::max(0,
		std::min(std::max(0, attack), std::max(0, defense)));
	const std::int32_t hit_count = HitCountForDamage(blocked_damage, maximum_hp);
	return MaximumDefenseExchangeDuration * hit_count / MaximumDamageHitCount;
}

[[nodiscard]] inline std::vector<std::int32_t> SplitDamage(
	const std::int32_t actual_damage, const std::int32_t maximum_hp) {
	const std::int32_t hit_count = HitCountForDamage(actual_damage, maximum_hp);
	if (hit_count <= 0) return {};
	std::vector<std::int32_t> hits(static_cast<std::size_t>(hit_count), actual_damage / hit_count);
	const std::int32_t remainder = actual_damage % hit_count;
	for (std::int32_t i = 0; i < remainder; ++i) {
		++hits[static_cast<std::size_t>(i)];
	}
	return hits;
}

[[nodiscard]] inline std::int32_t ApplyHit(
	const std::int32_t current_hp, const std::int32_t damage) noexcept {
	return std::max(0, current_hp - std::max(0, damage));
}

[[nodiscard]] inline bool ShouldContinueHits(const std::int32_t current_hp) noexcept {
	return 0 < current_hp;
}

[[nodiscard]] inline bool ShouldDrawHitEffect(
	const std::int32_t hit_count, const std::int32_t effect_x,
	const std::int32_t effect_y) noexcept {
	return (0 < hit_count) && (0 <= effect_x) && (0 <= effect_y);
}

} // namespace BattleDamageRules

#endif
