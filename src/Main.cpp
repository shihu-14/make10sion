# include <Siv3D.hpp> // Siv3D v0.6.16
# include "common.hpp"
# include "Title.hpp"
# include "Map.hpp"
# include "Battle.hpp"
# include "Result.hpp"
# include "Shop.hpp"
# include "Deck.hpp"

using namespace std;

void Main()
{
#ifndef NDEBUG
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

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Map>(State::Map);
	manager.add<Battle>(State::Battle);
	manager.add<Result>(State::Result);
	manager.add<Shop>(State::Shop);


	//XXX:debug用
	//開始シーンを指定する
	manager.init(State::Battle);

	while (System::Update() && manager.update()) {};
}
