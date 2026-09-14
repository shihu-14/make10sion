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
	bool preserve_deck_order = false;
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
		true,
		"../../image/boss_1.png",
		{
			"b5\n7$", "*$\n2+", "$+\n*3", "4/\n$6", "f\n3\n3", "q\n+",
			"*\n5", "o\n-", "i\ng\n-", "+\n2", "4\na", "e\nc",
			"+\nm", "6\n-", "/\n4", "3\n*", "i\n2", "h\n+",
		}
	};
	return scenario;
}

} // namespace DebugScenarioRules

#endif
