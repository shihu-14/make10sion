#ifndef ENEMY_INTENT_RULES_HPP
#define ENEMY_INTENT_RULES_HPP

#include <algorithm>
#include <cstdint>

namespace EnemyIntentRules {

struct Request {
	int32_t attack = 0;
	int32_t defense = 0;
	int32_t hand_limit = 0;
	int32_t hand_count = 0;
};

struct Intent {
	int32_t attack = 0;
	int32_t defense = 0;
	int32_t money_delta = 0;
	int32_t turn_advance = 0;
	int32_t action_cycle_override = 0;
	bool exit_requested = false;
	bool cancels_player_damage = false;
};

struct TurnState {
	bool has_action = false;
	bool side_effects_prepared = false;
	int32_t raw_attack = 0;
	int32_t raw_defense = 0;
};

inline void PrepareAction(TurnState& state, const int32_t raw_attack,
	const int32_t raw_defense) noexcept {
	if (state.has_action) return;
	state.has_action = true;
	state.raw_attack = raw_attack;
	state.raw_defense = raw_defense;
}

[[nodiscard]] inline Intent Resolve(const Request& request) noexcept {
	Intent result{ request.attack, request.defense };
	const int32_t played_count = std::max(request.hand_limit - request.hand_count, 0);
	switch (request.attack) {
	case -10: result.attack = 3 + 2 * played_count; break;
	case -11:
		result.attack = 20;
		result.turn_advance = 1;
		result.action_cycle_override = 4;
		break;
	case -12: result.attack = 60 - 4 * played_count; break;
	case -13:
		result.attack = 40;
		result.money_delta = -30;
		break;
	case -14:
		result.attack = 0;
		result.exit_requested = true;
		break;
	case -15: result.attack = 10 + 14 * played_count; break;
	case -16:
		result.attack = 30;
		result.turn_advance = 1;
		result.action_cycle_override = 5;
		break;
	case -17:
		result.attack = 80;
		result.cancels_player_damage = true;
		break;
	case -18: result.attack = 2 + 3 * played_count; break;
	case -19: result.attack = 3 + 5 * played_count; break;
	default: break;
	}
	return result;
}

[[nodiscard]] inline Intent ResolveFrame(bool& side_effects_prepared,
	const Request& request) noexcept {
	Intent result = Resolve(request);
	if (side_effects_prepared) {
		result.money_delta = 0;
		result.turn_advance = 0;
		result.action_cycle_override = 0;
	} else {
		side_effects_prepared = true;
	}
	return result;
}

} // namespace EnemyIntentRules

#endif
