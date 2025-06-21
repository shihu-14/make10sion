#ifndef Banner_HPP
#define Banner_HPP
#include <Siv3D.hpp> // Siv3D v0.6.16
#include "common.hpp"
#include "Deck.hpp"

class Banner {
private:
	//Write private functions or varables here.
	int money;
	int floor;

	bool isHovered_deck = false;
	double deck_alpha = 0.0;
	bool isHovered_setting = false;
	double setting_alpha = 0.0;

	Deck deck; // Deckクラスのインスタンス
	bool deck_mode = false; // デッキモードのフラグ

	const Texture floor_img{ U"../../image/UI_floor_hyouzi.png" };
	const Texture money_img{ U"../../image/UI_money.png" };
	const Texture setting_img{ U"../../image/bottun_option.png" };
	const Texture deck_img{ U"../../image/bottun_deck.png" };
	const Font fontBitMap{ 48 };
	const Font fontBitMap2{ 96,Typeface::Bold };
public:
	void init(int global_money, int global_floor);
	bool update(std::vector<Block>& deck_data);
	void draw() const;
};

#endif
