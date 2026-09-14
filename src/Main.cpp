# include <Siv3D.hpp> // Siv3D v0.6.16
# include "common.hpp"
# include "Title.hpp"
# include "Map.hpp"
# include "Battle.hpp"
# include "Result.hpp"
# include "Shop.hpp"
# include "Event.hpp"
# include "Deck.hpp"
#if defined(DEBUG) || defined(_DEBUG)
# include "DebugScenarioFactory.hpp"
#endif

using namespace std;

void Main()
{
#if defined(DEBUG) || defined(_DEBUG)
	Logger << U"make10sion module: " << FileSystem::ModulePath();
	Logger << U"make10sion working directory: " << FileSystem::CurrentDirectory();
#endif
	Scene::SetBackground(ColorF{ 0.0, 0.0, 0.0 });
	//windowsサイズ
	Window::Resize(1920, 1080);
	Scene::SetResizeMode(ResizeMode::Keep);
	Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720);
	//フルスクリーン
	//Window::SetFullscreen(true);
	//タイトル
	Window::SetTitle(U"Arithmancer");

	auto shared_data = std::make_shared<GameData>();
	State initial_state = State::Title;
#if defined(DEBUG) || defined(_DEBUG)
	if (System::GetCommandLineArgs().includes(U"--debug-midgame")) {
		Reseed(DebugScenarioRules::Midgame().seed);
		shared_data = DebugScenarioFactory::CreateMidgame();
		initial_state = State::Battle;
		Logger << U"make10sion debug scenario: midgame, layer=" << shared_data->Layer
			<< U", unlocked=" << shared_data->board_progress.UnlockedCount()
			<< U", hand_limit=" << GameStateRules::ResolveBattleHandLimit(
				shared_data->board_progress,
				shared_data->debug_battle_overrides ? shared_data->debug_battle_overrides->hand_limit : 0)
			<< U", enemy_texture=" << shared_data->debug_battle_overrides->enemy_texture_path
			<< U", deck_size=" << shared_data->Deck.size();
	}
#endif
	App manager{ shared_data };
	manager.add<Title>(State::Title);
	manager.add<Map>(State::Map);
	manager.add<Battle>(State::Battle);
	manager.add<Result>(State::Result);
	manager.add<Shop>(State::Shop);
	manager.add<Event>(State::Event);


	manager.init(initial_state);

	while (System::Update() && manager.update()) {};
}
