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
    /////////////////////////////////////////////////////////////////////
    if (!void_normal_1) {
        bool isHovered_normal_1 = RoundRect{ 350, 500, 300, 100,20 }.mouseOver();
        //カードの購入処理
        if (isHovered_normal_1 && MouseL.down()) {
            if (getData().money >= 50) {
                getData().Deck.push_back(normal_1);
                getData().money -= 50;
                //Shopのカードを更新
                void_normal_1 = true;
                normal_1_alpha = 0.8; // 売り切れ
                banner.init(getData().money, getData().Layer);
            } else {
                //TODO:効果音！！
            }
            return;
        }
        if (normal_1_alpha < 0.4 && isHovered_normal_1) {
            normal_1_alpha += 0.1;
            if (normal_1_alpha > 0.4) normal_1_alpha = 0.4;
        } else if (normal_1_alpha > 0.0 && !isHovered_normal_1) {
            normal_1_alpha -= 0.1;
            if (normal_1_alpha < 0.0) normal_1_alpha = 0.0;
        }
    }
    ////////////////////////////////////////////////////////////////////
    if (!void_normal_2) {
        bool isHovered_normal_2 = RoundRect{ 650, 500, 300, 100,20 }.mouseOver();
        //カードの購入処理
        if (isHovered_normal_2 && MouseL.down()) {
            if (getData().money >= 50) {
                getData().Deck.push_back(normal_2);
                getData().money -= 50;
                //Shopのカードを更新
                void_normal_2 = true;
                normal_2_alpha = 0.8; // 売り切れ
                banner.init(getData().money, getData().Layer);
            } else {
            }
            return;
        }
        if (normal_2_alpha < 0.4 && isHovered_normal_2) {
            normal_2_alpha += 0.1;
            if (normal_2_alpha > 0.4) normal_2_alpha = 0.4;
        } else if (normal_2_alpha > 0.0 && !isHovered_normal_2) {
            normal_2_alpha -= 0.1;
            if (normal_2_alpha < 0.0) normal_2_alpha = 0.0;
        }
    }
    ////////////////////////////////////////////////////////////////////
    if (!void_uncommon) {
        bool isHovered_uncommon = RoundRect{ 950, 500, 300, 100,20 }.mouseOver();
        //カードの購入処理
        if (isHovered_uncommon && MouseL.down()) {
            if (getData().money >= 100) {
                getData().Deck.push_back(uncommon);
                getData().money -= 100;
                //Shopのカードを更新
                void_uncommon = true;
                uncommon_alpha = 0.8; // 売り切れ
                banner.init(getData().money, getData().Layer);
            } else {
            }
            return;
        }
        if (uncommon_alpha < 0.4 && isHovered_uncommon) {
            uncommon_alpha += 0.1;
            if (uncommon_alpha > 0.4) uncommon_alpha = 0.4;
        } else if (uncommon_alpha > 0.0 && !isHovered_uncommon) {
            uncommon_alpha -= 0.1;
            if (uncommon_alpha < 0.0) uncommon_alpha = 0.0;
        }
    }
    ////////////////////////////////////////////////////////////////////
    if (!void_rare) {
        bool isHovered_rare = RoundRect{ 1250, 500, 300, 100,20 }.mouseOver();
        //カードの購入処理
        if (isHovered_rare && MouseL.down()) {
            if (getData().money >= 150) {
                getData().Deck.push_back(rare);
                getData().money -= 150;
                //Shopのカードを更新
                void_rare = true;
                rare_alpha = 0.8; // 売り切れ
                banner.init(getData().money, getData().Layer);
            } else {
            }
            return;
        }
        if (rare_alpha < 0.4 && isHovered_rare) {
            rare_alpha += 0.1;
            if (rare_alpha > 0.4) rare_alpha = 0.4;
        } else if (rare_alpha > 0.0 && !isHovered_rare) {
            rare_alpha -= 0.1;
            if (rare_alpha < 0.0) rare_alpha = 0.0;
        }
    }
    ////////////////////////////////////////////////////////////////////


    //戻るボタンの更新
    if (MouseL.down() && RectF { 1600, 800, 225, 225 }.mouseOver()) {
        //TODO: 戻るボタンが押された場合の処理
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
        // 背景の描画
        background_img.draw(0, 0);
        //商品(カード)の描画

        normal_1.Draw({ 500, 300 }, 1.5, 0.0, 1.0);
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - normal_1_alpha, 1.0 - normal_1_alpha, 1.0 - normal_1_alpha } };
            double scale = 1.0 - ((normal_1_alpha <= 0.4) ? (normal_1_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            price_img.scaled(scale).drawAt(500, 650);
            fontBitMap(U"50G").drawAt(520, 550, money_check(50));
        }

        normal_2.Draw({ 800, 300 }, 1.5, 0.0, 1.0);
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - normal_2_alpha, 1.0 - normal_2_alpha, 1.0 - normal_2_alpha } };
            double scale = 1.0 - ((normal_2_alpha <= 0.4) ? (normal_2_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            price_img.scaled(scale).drawAt(800, 650);
            fontBitMap(U"50G").drawAt(820, 550, money_check(50));
        }


        uncommon.Draw({ 1100, 300 }, 1.5, 0.0, 1.0);
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - uncommon_alpha, 1.0 - uncommon_alpha, 1.0 - uncommon_alpha } };
            double scale = 1.0 - ((uncommon_alpha <= 0.4) ? (uncommon_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            price_img.scaled(scale).drawAt(1100, 650);
            fontBitMap(U"100G").drawAt(1120, 550, money_check(100));
        }

        rare.Draw({ 1400, 300 }, 1.5, 0.0, 1.0);
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - rare_alpha, 1.0 - rare_alpha, 1.0 - rare_alpha } };
            double scale = 1.0 - ((rare_alpha <= 0.4) ? (rare_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            price_img.scaled(scale).drawAt(1400, 650);
            fontBitMap(U"150G").drawAt(1420, 550, money_check(150));
        }

        //戻るボタン
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - return_alpha, 1.0 - return_alpha, 1.0 - return_alpha } };
            double scale = 1.0 - ((return_alpha <= 0.4) ? (return_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            back_button_img.scaled(0.75 * scale).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
        }
    }
    // バナーの描画
    banner.draw();
}

void Shop::drawFadeIn(double t) const {
    // 背景の描画
    background_img.draw(0, 0);
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
