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
	int32_t hand_limit_override = 0;
	std::string_view enemy_texture_path;
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
		18,
		"../../image/boss_1.png",
		{
			"0", "2", "3", "4", "5", "6", "7",
			"+", "-", "*", "/",
			"g", "h", "e", "b", "i", "f", "q",
		}
	};
	return scenario;
}

} // namespace DebugScenarioRules

#endif
