#ifndef DEBUG_SCENARIO_FACTORY_HPP
#define DEBUG_SCENARIO_FACTORY_HPP

#include "common.hpp"
#include "DebugScenarioRules.hpp"

#include <memory>

namespace DebugScenarioFactory {

inline std::shared_ptr<GameData> CreateMidgame()
{
	auto data = std::make_shared<GameData>();
	data->ResetForNewRun();
	const auto& scenario = DebugScenarioRules::Midgame();
	data->Layer = scenario.layer;
	data->Index = 1;
	data->HP = scenario.hp;
	data->MaxHP = scenario.max_hp;
	data->money = scenario.money;
	data->enemy = scenario.enemy_type;
	data->debug_battle_overrides = GameData::DebugBattleOverrides{
		scenario.hand_limit_override,
		scenario.preserve_deck_order,
		Unicode::Widen(std::string{ scenario.enemy_texture_path }),
	};
	data->Deck.clear();
	data->Deck.reserve(scenario.deck.size());
	for (const auto definition : scenario.deck) data->Deck.emplace_back(std::string{ definition });
	data->board_progress.UnlockAll();
	return data;
}

}

#endif
