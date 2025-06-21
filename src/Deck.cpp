#include "Deck.hpp"
#include "common.hpp"
#define M_PI 3.14159265358979323846
using namespace std;

void Deck::init(vector<Block>& deck) {
	deck_size = (int)deck.size(); //デッキのサイズを設定
	deck_data = deck; //デッキのカードを設定
	card_pos.clear(); //カードの位置を初期化
	for (int i = 0;i < deck_size;i++)
		card_pos.push_back({ 400 + (i % 6) * 200, 300 + 250 * (i / 6) }); //カードの位置を設定
	card_fade.clear(); //カードのフェードを初期化
	card_fade.resize(deck_size, 0.0); //カードのフェードを1.0に設定
	int tmp = max(0, ((deck_size - 1) / 6) - 3);
	max_y = max(0, (tmp == 0) ? 0 : 100 + 250 * tmp); //最大のy座標を設定
	now_y = 0; //現在のy座標を初期化
	fade_alpha = 0.0; //フェードの透明度を初期化
	first_call = false; //初回呼び出しフラグを初期化
	fade_mode = true; //フェードインモードを有効にする
	timer = 0; //フェードインのタイマーを初期化
}

bool Deck::update() {
	if (!first_call) {
		first_call = true;
		timer = (int)Time::GetMillisec();
	}
	if (timer + 1000 > Time::GetMillisec()) {
		updateFadeIn((double)(Time::GetMillisec() - timer) / 1000.0);
		fade_mode = true;
	} else {
		if (fade_mode)fade_mode = false;
		const double wheel = Mouse::Wheel();
		now_y = Clamp(now_y - wheel * 75, -max_y, 0.0); //ホイールでスクロール
		is_pushed = (RectF{ 1600, 800, 225, 225 }.mouseOver());
		if (is_pushed && MouseL.up()) return false; //戻るボタンが押された場合はfalseを返す

		if (fade_alpha < 0.4 && is_pushed) {
			fade_alpha += 0.1;
			if (fade_alpha > 0.4) fade_alpha = 0.4;
		} else if (fade_alpha > 0.0 && !is_pushed) {
			fade_alpha -= 0.1;
			if (fade_alpha < 0.0) fade_alpha = 0.0;
		}
	}
	return true;
}

void Deck::draw() const {
	//背景を描画
	background_img.draw(0, 0, ColorF{ 1.0, 1.0, 1.0 });
	//デッキ画像を描画
	for (int i = 0;i < deck_size;i++) {
		if (fade_mode) {
			deck_data.at(i).Draw({ card_pos.at(i).first, card_pos.at(i).second + now_y }, card_fade.at(i), M_PI * (1.0 - card_fade.at(i)), card_fade.at(i));
		} else {
			deck_data.at(i).Draw({ card_pos.at(i).first, card_pos.at(i).second + now_y }, 1.0, 0.0, 1.0);
		}
	}
	//戻るボタン
	{
		const ScopedColorMul2D colorMul{ ColorF{ 1.0 - fade_alpha, 1.0 - fade_alpha, 1.0 - fade_alpha } };
		double scale = 1.0 - ((fade_alpha <= 0.4) ? (fade_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
		back_button_img.scaled(0.75 * scale).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
	}
}

void Deck::updateFadeIn(double t) {
	for (int i = 0;i < deck_size;i++) {
		const double progree = EaseInOutExpo(Clamp(0.05 * (i + 6) - t, 0.0, 0.8));
		card_fade.at(i) = 1.0 - Math::Lerp(0.0, 1.0, progree);
	}

}