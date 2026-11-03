# include <Siv3D.hpp> // Siv3D v0.6.16
# include "common.hpp"
# include "Title.hpp"
# include "Map.hpp"
# include "Battle.hpp"
# include "Result.hpp"
# include "Shop.hpp"
# include "Event.hpp"
# include "Deck.hpp"
# include "DemoScenarioFactory.hpp"

using namespace std;

void Main()
{
	Scene::SetBackground(ColorF{ 0.0, 0.0, 0.0 });
	//windowsサイズ
	Window::Resize(1920, 1080);
	Scene::SetResizeMode(ResizeMode::Keep);
	Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720);
	//フルスクリーン
	//Window::SetFullscreen(true);
	//タイトル
	Window::SetTitle(U"make10sion");

	auto shared_data = std::make_shared<GameData>();
	State initial_state = State::Title;
	if (System::GetCommandLineArgs().includes(U"--demo")) {
		shared_data = DemoScenarioFactory::CreateMidgame();
		initial_state = State::Battle;
	}
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
