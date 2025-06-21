# pragma once
# include <Siv3D.hpp>
# include <vector>
# include "Block.hpp"
# include "Board.hpp"

// シーンの名前
enum class State
{
	Title,
	Battle,
	Map,
	Result
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
};

using App = SceneManager<State, GameData>;