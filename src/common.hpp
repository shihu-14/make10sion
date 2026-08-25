# pragma once
# include <Siv3D.hpp>
# include <vector>
# include "Block.hpp"
# include "GameStateRules.hpp"
# include "leric.hpp"

// シーンの名前
enum class State
{
	Title,
	Battle,
	Map,
	Result,
	Shop,
	Event
};

// マップのポイントタイプ
enum class MapPointType {
	Boss,
	Elite,
	Event,
	Shop,
	Enemy,
	Treasure,
	None,
};
// マップのノードデータ
struct Node {
	MapPointType type = MapPointType::None;
	int NextLayerIndex = 0; // 次の層のインデックス (1:上層, 2:中層, 4:下層)
};

inline std::vector<Block> CreateStarterDeck()
{
	return {
		Block("2\n3"),
		Block("2\n3"),
		Block("3\n2"),
		Block("3\n2"),
		Block("2\n3"),
		Block("q\nj"),
		Block("+\n+"),
		Block("+\n+"),
		Block("+\n+"),
		Block("+\n*"),
		Block("+\n*")
	};
}

// 共有するデータ
struct GameData
{
	struct DebugBattleOverrides {
		int hand_limit = 0;
		bool preserve_deck_order = false;
		String enemy_texture_path;
	};

	std::vector<Block> Deck = CreateStarterDeck();
	int Layer = 0;
	int Index = 1; // 現在のマップのインデックス
	int HP = 80;
	int MaxHP = 80;
	int money = 100;
	GameStateRules::BoardProgress board_progress;
	GameStateRules::RunOutcome run_outcome = GameStateRules::RunOutcome::None;
	GameStateRules::AudioSettings audio_settings;
	int enemy = 0;
	Optional<DebugBattleOverrides> debug_battle_overrides;
	Leric leric; // レリック

	// Map Data
	std::vector<std::vector<Node>> selected_nodes;
	int selected_map_act = -1;

	//Shop のカードデータ
	// 通常カード
	std::vector<Block> normal_cards = {
		Block("4\n4"),
		Block("o\np"),
		Block("0\n7"),
		Block("3\n2\n7\n1"),
		Block("12\n4$"),
		Block("1$\n4d"),
		Block("d\n2\nd"),
		Block("*3\n3$"),
		Block("+\n+\n+"),
		Block("+6\n+$"),
		Block("+\nd\n+$"),
		Block("+d\n+$"),
		Block("*\n-"),
		Block("*\n/"),
		Block("+\n+"),
		Block("-\n+\n*\n+"),
		Block("$+$\nd*3\n$-$")
	};
	// 特殊カード
	std::vector<Block> unccommon_cards = {
		Block("g\nh"),
		Block("m\nn\nn"),
		Block("i\n3"),
		Block("b\na"),
		Block("E\n6"),
		Block("5\n5"),
		Block("f\n3"),
		Block("j\nk"),
		Block("*\n+"),
		Block("*\n+"),
		Block("*\n3"),
		Block("*\n3"),
		Block("+d\n*$"),
		Block("+d\n*$"),
		Block("-\n-"),
		Block("-\n-"),
		Block("*n\n*$"),
		Block("*n\n*$")
	};
	// レアカード
	std::vector<Block> rare_cards = {
		Block("g\ne"),
		Block("c\n3"),
		Block("7\n3"),
		Block("6\n6"),
		Block("*\n*"),
		Block("*"),
		Block("*\n*\n/"),
		Block("*\n-\n*"),
		Block("*4\n*$")
	};

	void ResetForNewRun()
	{
		Deck = CreateStarterDeck();
		Layer = 0;
		Index = 1;
		HP = 80;
		MaxHP = 80;
		money = 100;
		board_progress.Reset();
		run_outcome = GameStateRules::RunOutcome::None;
		enemy = 0;
		debug_battle_overrides.reset();
		leric.Reset();
		selected_nodes.clear();
		selected_map_act = -1;
	}
};

using App = SceneManager<State, GameData>;
