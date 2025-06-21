#ifndef Shop_HPP
#define Shop_HPP
#include <Siv3D.hpp> // Siv3D v0.6.16
#include "common.hpp"
#include "Deck.hpp"
#include "Banner.hpp"


class Shop : public App::Scene {
private:
	Banner banner;
	bool deck_mode = false; // デッキモードのフラグ
	double return_alpha = 0.0; // 戻るボタンのアルファ値
	double normal_1_alpha = 0.0; // 通常カード1のアルファ値
	double normal_2_alpha = 0.0; // 通常カード2のアルファ値
	double uncommon_alpha = 0.0; // 特殊カードのアルファ値
	double rare_alpha = 0.0; // レアカードのアルファ値

	bool void_normal_1 = false; // 通常カード1が空かどうか
	bool void_normal_2 = false; // 通常カード2が空かどうか
	bool void_uncommon = false; // 特殊カードが空かどうか
	bool void_rare = false; // レアカードが空かどうか

	Block normal_1;
	Block normal_2;
	Block uncommon;
	Block rare;
	const Texture price_img{ U"../../image/UI_money.png" };
	const Texture back_button_img{ U"../../image/back_button_deck0.png" };
	const Texture background_img{ U"../../image/UI_shop.png" };

	const Font fontBitMap{ 48 };
public:
	Shop(const InitData& init);


	void update() override;
	void draw() const override;

	//void updateFadeIn(double t) override ;
	void drawFadeIn(double t) const override;
};

#endif
