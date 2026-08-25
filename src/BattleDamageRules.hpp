#ifndef BattleDamageRules_HPP
#define BattleDamageRules_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

namespace BattleDamageRules {

[[nodiscard]] inline std::int32_t HitCountForDamage(
	const std::int32_t actual_damage, const std::int32_t maximum_hp) noexcept {
	if ((actual_damage <= 0) || (maximum_hp <= 0)) return 0;
	const std::int64_t scaled_damage = static_cast<std::int64_t>(actual_damage) * 100;
	const std::int64_t scaled_maximum = maximum_hp;
	std::int32_t count = 5;
	if (scaled_damage <= 15 * scaled_maximum) count = 1;
	else if (scaled_damage <= 30 * scaled_maximum) count = 2;
	else if (scaled_damage <= 45 * scaled_maximum) count = 3;
	else if (scaled_damage <= 60 * scaled_maximum) count = 4;
	return std::min(count, actual_damage);
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
