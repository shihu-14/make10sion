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

	Block normal_1;
	Block normal_2;
	Block uncommon;
	Block rare;
	const Texture price_img{ U"../../image/UI_money.png" };
	const Texture back_button_img{ U"../../image/back_button_deck0.png" };

	const Font fontBitMap{ 48 };
public:
	Shop(const InitData& init);


	void update() override;
	void draw() const override;

	//void updateFadeIn(double t) override ;
	void drawFadeIn(double t) const override;
};

#endif
