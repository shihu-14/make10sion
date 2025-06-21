#include "Shop.hpp"
#define M_PI 3.14159265358979323846
using namespace std;

Shop::Shop(const InitData& init) : IScene(init) {
    banner.init(getData().money, getData().Layer);

    normal_1 = getData().normal_cards[Random<int>(0, (int)getData().normal_cards.size() - 1)];
    normal_2 = getData().normal_cards[Random<int>(0, (int)getData().normal_cards.size() - 1)];
    uncommon = getData().unccommon_cards[Random<int>(0, (int)getData().unccommon_cards.size() - 1)];
    rare = getData().rare_cards[Random<int>(0, (int)getData().rare_cards.size() - 1)];
    // TODO:エリックも追加する
}



void Shop::update() {
    deck_mode = banner.update(getData().Deck);
    if (deck_mode)return;
    //戻るボタンの更新
    if (MouseL.down() && RectF { 1600, 800, 225, 225 }.mouseOver()) {
        //TOODO: 戻るボタンが押された場合の処理
        return;
    }
    if (return_alpha < 0.4 && RectF{ 1600, 800, 225, 225 }.mouseOver()) {
        return_alpha += 0.1;
        if (return_alpha > 0.4) return_alpha = 0.4;
    } else if (return_alpha > 0.0 && !RectF{ 1600, 800, 225, 225 }.mouseOver()) {
        return_alpha -= 0.1;
        if (return_alpha < 0.0) return_alpha = 0.0;
    }
}

#define money_check(price) (getData().money >= price ? Palette::White : Palette::Red)

void Shop::draw() const {
    if (!deck_mode) {
        //商品(カード)の描画
        normal_1.Draw({ 500, 300 }, 1.5, 0.0, 1.0);
        price_img.drawAt(500, 650);
        fontBitMap(U"50G").drawAt(520, 550, money_check(50));

        normal_2.Draw({ 800, 300 }, 1.5, 0.0, 1.0);
        price_img.drawAt(800, 650);
        fontBitMap(U"50G").drawAt(820, 550, money_check(50));

        uncommon.Draw({ 1100, 300 }, 1.5, 0.0, 1.0);
        price_img.drawAt(1100, 650);
        fontBitMap(U"100G").drawAt(1120, 550, money_check(100));

        rare.Draw({ 1400, 300 }, 1.5, 0.0, 1.0);
        price_img.drawAt(1400, 650);
        fontBitMap(U"150G").drawAt(1420, 550, money_check(150));

        //戻るボタン
        back_button_img.scaled(0.75).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
        RectF{ 1600, 800, 225, 225 }.draw(ColorF{ 0.0, 0.0, 0.0, return_alpha });
    }
    // バナーの描画
    banner.draw();
}

void Shop::drawFadeIn(double t) const {
    // フェードインの描画処理
    double time = Clamp(t, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    normal_1.Draw({ 500, 300 }, time * 1.5, M_PI * (1.0 - time), time);
    price_img.drawAt(500, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap(U"50G").drawAt(520, 550 + 50 * (1.0 - time), ColorF{ money_check(50), time });

    time = Clamp(t - 0.2, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    normal_2.Draw({ 800, 300 }, time * 1.5, M_PI * (1.0 - time), time);
    price_img.drawAt(800, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap(U"50G").drawAt(820, 550 + 50 * (1.0 - time), ColorF{ money_check(50), time });

    time = Clamp(t - 0.4, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    uncommon.Draw({ 1100, 300 }, time * 1.5, M_PI * (1.0 - time), time);
    price_img.drawAt(1100, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap(U"100G").drawAt(1120, 550 + 50 * (1.0 - time), ColorF{ money_check(100), time });

    time = Clamp(t - 0.6, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    rare.Draw({ 1400, 300 }, time * 1.5, M_PI * (1.0 - time), time);
    price_img.drawAt(1400, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap(U"150G").drawAt(1420, 550 + 50 * (1.0 - time), ColorF{ money_check(150), time });

    // 戻るボタンの描画
    back_button_img.scaled(0.75).draw(1600, 800 + 50 * (1.0 - t), ColorF{ 1.0, 1.0, 1.0, time });

    // バナーの描画
    banner.draw();
}
#undef money_check
