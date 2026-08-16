#ifndef SHOP_RULES_HPP
#define SHOP_RULES_HPP

namespace ShopRules {

[[nodiscard]] constexpr bool IsOneTimeRelic(const int relic_index) noexcept {
	return (relic_index == 3) || (relic_index == 10)
		|| (relic_index == 11) || (relic_index == 18);
}

[[nodiscard]] constexpr bool CanOfferRelic(const int relic_index,
	const int owned_count, const bool offered_in_this_shop) noexcept {
	return !IsOneTimeRelic(relic_index) || ((owned_count == 0) && !offered_in_this_shop);
}

} // namespace ShopRules

#endif
