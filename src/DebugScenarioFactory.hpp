#ifndef DEBUG_SCENARIO_FACTORY_HPP
#define DEBUG_SCENARIO_FACTORY_HPP

#include "common.hpp"

#include <memory>

namespace DebugScenarioFactory {

inline std::shared_ptr<GameData> CreateMidgame()
{
	auto data = std::make_shared<GameData>();
	data->ResetForNewRun();
	data->Layer = 12;
	data->Index = 1;
	data->HP = 60;
	data->money = 200;
	data->enemy = 0;
	data->Deck.push_back(data->normal_cards.front());
	data->Deck.push_back(data->unccommon_cards.front());
	data->leric.getLeric().at(13) = 1;
	data->leric.getLeric().at(14) = 1;
	while (data->board_progress.UnlockedCount() < 12) {
		const auto candidates = data->board_progress.UnlockableCells();
		if (candidates.empty() || !data->board_progress.Unlock(candidates.front())) break;
	}
	return data;
}

}

#endif
