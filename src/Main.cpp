# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Block.hpp"
//TODO:ここにBoard.hppを追加!!

// シーンの名前
enum class State
{
	Title,
	Battle
};

// 共有するデータ
struct GameData
{
	std::vector<Block> Deck;
	int Layer = 0;
	int HP = 80;
	int MaxHP = 80;
	int money = 0;
	Board board;
	long long status = 0; // 状態
};

using App = SceneManager<State, GameData>;


# include "Title.hpp"
# include "Battle.hpp"

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
	Window::SetTitle(U"ShoutWars");

	App manager;
	manager.add<Title>(State::Title);
	manager.add<Battle>(State::Battle);
	

	//XXX:debug用
	//開始シーンを指定する
	manager.init(State::Title);

	while (System::Update() && manager.update()) {};
}
