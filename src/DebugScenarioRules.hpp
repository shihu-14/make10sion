#ifndef DEBUG_SCENARIO_RULES_HPP
#define DEBUG_SCENARIO_RULES_HPP

#include <cstdint>
#include <string_view>
#include <vector>

namespace DebugScenarioRules {

struct Scenario {
	int32_t layer = 0;
	int32_t enemy_type = 0;
	int32_t hp = 0;
	int32_t max_hp = 0;
	int32_t money = 0;
	uint64_t seed = 0;
	std::vector<std::string_view> deck;
};

[[nodiscard]] inline const Scenario& Midgame() {
	static const Scenario scenario{
		22,
		0,
		100,
		100,
		300,
		0x4D313053ULL,
		{
			"2\n3",
			"0\n7",
			"-\n+\n*\n+",
			"*\n*\n/",
			"g\nh",
			"g\ne",
			"b\na",
			"c\n3",
			"i\n3",
			"f\n3",
			"j\nk",
			"o\np",
			"q\nj",
			"m\nn\nn",
			"E\n6",
		}
	};
	return scenario;
}

} // namespace DebugScenarioRules

#endif
