#ifndef Deck_HPP
#define Deck_HPP
#include <Siv3D.hpp> // Siv3D v0.6.16
#include "common.hpp"
#include <vector>

class Deck {
private:
	std::vector<std::pair<int, int>> card_pos; //カードの位置
	std::vector<double> card_fade; //カードのsize,alpha
	double max_y = 0;
	double now_y = 0; //現在のy座標
	bool is_pushed = false; //ボタンが押されているか
	double fade_alpha = 0.0; //フェードの透明度
	const Texture background_img{ U"../../image/deck_background.png" };
	const Texture back_button_img{ U"../../image/back_button_deck0.png" };
	int deck_size = 0; //デッキのサイズ
	std::vector<Block> deck_data; //デッキのカード
	bool first_call = false;
	bool fade_mode = true; //フェードインモード
	int timer = 0; //フェードインのタイマー
	void updateFadeIn(double t);
public:
	void init(std::vector<Block>& deck);
	bool update();
	void draw() const;
};

#endif
