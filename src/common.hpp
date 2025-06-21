# pragma once
# include <Siv3D.hpp>
# include <vector>
# include "Block.hpp"
//# include "Board.hpp"

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
	//Board board;
	long long status = 0; // 状態
};

using App = SceneManager<State, GameData>;