#include "Deck.hpp"
#include "common.hpp"
#define M_PI 3.14159265358979323846
using namespace std;

Deck::Deck(const InitData& init) : IScene(init) {
	//XXX:debug用
	getData().Deck.resize(getData().rare_cards.size()); //デッキのサイズを通常カードの数に合わせる
	getData().Deck = getData().rare_cards; //デッキに通常カードをセット

	card_pos.clear(); //カードの位置を初期化
	for (int i = 0;i < getData().Deck.size();i++)
		card_pos.push_back({ 400 + (i % 6) * 200, 200 + 250 * (i / 6) }); //カードの位置を設定
	card_fade.clear(); //カードのフェードを初期化
	card_fade.resize(getData().Deck.size(), 0.0); //カードのフェードを1.0に設定
	int tmp = max(0ull, ((getData().Deck.size() - 1) / 6) - 3);
	max_y = max(0, (tmp == 0) ? 0 : 100 + 250 * tmp); //最大のy座標を設定
}



void Deck::update() {
	const double wheel = Mouse::Wheel();
	now_y = Clamp(now_y - wheel * 75, -max_y, 0.0); //ホイールでスクロール
	is_pushed = (RectF{ 1600, 800, 225, 225 }.mouseOver());
	if (fade_alpha < 0.4 && is_pushed) {
		fade_alpha += 0.1;
		if (fade_alpha > 0.4) fade_alpha = 0.4;
	} else if (fade_alpha > 0.0 && !is_pushed) {
		fade_alpha -= 0.1;
		if (fade_alpha < 0.0) fade_alpha = 0.0;
	}
}

void Deck::draw() const {
	background_img.draw(0, 0, ColorF{ 1.0, 1.0, 1.0 });
	for (int i = 0;i < getData().Deck.size();i++)
		getData().Deck.at(i).Draw({ card_pos.at(i).first, card_pos.at(i).second + now_y }, 1.0, 0.0, 1.0);

	back_button_img.scaled(0.75).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
	RectF{ 1600, 800, 225, 225 }.draw(ColorF{ 0.0, 0.0, 0.0, fade_alpha });
}

void Deck::updateFadeIn(double t) {
	for (int i = 0;i < getData().Deck.size();i++) {
		const double progree = EaseInOutExpo(Clamp(0.05 * (i + 6) - t, 0.0, 0.8));
		card_fade.at(i) = 1.0 - Math::Lerp(0.0, 1.0, progree);
	}

}

void Deck::drawFadeIn(double t) const {
	background_img.draw(0, 0, ColorF{ 1.0, 1.0, 1.0 });
	for (int i = 0;i < getData().Deck.size();i++)
		getData().Deck.at(i).Draw({ card_pos.at(i).first, card_pos.at(i).second + now_y }, 1.0, 0.0, card_fade.at(i));

	back_button_img.scaled(0.75).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
	RectF{ 1600, 800, 225, 225 }.draw(ColorF{ 0.0, 0.0, 0.0, fade_alpha });
}
