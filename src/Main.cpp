# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Block.hpp"
# include "Board.hpp"

// シーンの名前
enum class State
{
	Title,
	Battle,
	Map,
	Result,
	Deck
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
# include "Map.hpp"
# include "Battle.hpp"
# include "Result.hpp"
# include "Deck.hpp"

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
	manager.add<Map>(State::Map);
	manager.add<Battle>(State::Battle);
	manager.add<Result>(State::Result);
	manager.add<Deck>(State::Deck);
	

	//XXX:debug用
	//開始シーンを指定する
	manager.init(State::Title);

	while (System::Update() && manager.update()) {};
}
