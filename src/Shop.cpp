#include "Shop.hpp"
#include "ShopRules.hpp"
using namespace std;

Shop::Shop(const InitData& init) : IScene(init), leric_alpha(4, 0.0), void_leric(4, false), leric_index(4, 0) {

    normal_1 = getData().normal_cards[Random<int>(0, (int)getData().normal_cards.size() - 1)];
    normal_2 = getData().normal_cards[Random<int>(0, (int)getData().normal_cards.size() - 1)];
    uncommon = getData().unccommon_cards[Random<int>(0, (int)getData().unccommon_cards.size() - 1)];
    rare = getData().rare_cards[Random<int>(0, (int)getData().rare_cards.size() - 1)];

    if (getData().leric.getLeric().at(18))discount = 0.8;

    vector<int> available_lerics = { 3,5,10,11,13,14,15,16,18 };
    available_lerics.erase(std::remove_if(available_lerics.begin(), available_lerics.end(),
        [this](const int relic_index) {
            return !ShopRules::CanOfferRelic(relic_index,
                getData().leric.getLeric().at(relic_index), false);
        }), available_lerics.end());
    for (int i = 0; i < 4; i++) {
        int index = Random<int>(0, (int)available_lerics.size() - 1);
        leric_index[i] = available_lerics[index];
        if (ShopRules::IsOneTimeRelic(leric_index[i])) {
            available_lerics.erase(available_lerics.begin() + index);
        }
    }
}



void Shop::update() {
    deck_mode = banner.update(getData().Deck);
    if (deck_mode)return;
    /////////////////////////////////////////////////////////////////////
    if (!void_normal_1) {
        bool isHovered_normal_1 = RoundRect{ 350, 500, 300, 100,20 }.mouseOver();
        if (isHovered_normal_1)Cursor::RequestStyle(CursorStyle::Hand);
        //カードの購入処理
        if (isHovered_normal_1 && MouseL.down()) {
            if (getData().money >= discount * 50) {
                buy_se.play(); // 購入音を再生
                getData().Deck.push_back(normal_1);
                getData().money -= discount * 50;
                //Shopのカードを更新
                void_normal_1 = true;
                normal_1_alpha = 0.8; // 売り切れ
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
        if (isHovered_normal_2)Cursor::RequestStyle(CursorStyle::Hand);
        //カードの購入処理
        if (isHovered_normal_2 && MouseL.down()) {
            if (getData().money >= discount * 50) {
                buy_se.play(); // 購入音を再生
                getData().Deck.push_back(normal_2);
                getData().money -= discount * 50;
                //Shopのカードを更新
                void_normal_2 = true;
                normal_2_alpha = 0.8; // 売り切れ
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
        if (isHovered_uncommon)Cursor::RequestStyle(CursorStyle::Hand);
        //カードの購入処理
        if (isHovered_uncommon && MouseL.down()) {
            if (getData().money >= discount * 100) {
                buy_se.play(); // 購入音を再生
                getData().Deck.push_back(uncommon);
                getData().money -= discount * 100;
                //Shopのカードを更新
                void_uncommon = true;
                uncommon_alpha = 0.8; // 売り切れ
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
        if (isHovered_rare)Cursor::RequestStyle(CursorStyle::Hand);
        if (isHovered_rare && MouseL.down()) {
            if (getData().money >= discount * 150) {
                buy_se.play(); // 購入音を再生
                getData().Deck.push_back(rare);
                getData().money -= discount * 150;
                //Shopのカードを更新
                void_rare = true;
                rare_alpha = 0.8; // 売り切れ
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
    for (int i = 0; i < 4; i++) {
        if (!void_leric[i]) {
            bool isHovered_leric = RoundRect{ 350 + 300 * i, 850, 300, 100,20 }.mouseOver();
            if (isHovered_leric)Cursor::RequestStyle(CursorStyle::Hand);
            //レリックの購入処理
            if (isHovered_leric && MouseL.down()) {
                if (getData().money >= discount * 150) {
                    buy_se.play(); // 購入音を再生
                    getData().leric.getLeric().at(leric_index[i])++;
                    getData().money -= discount * 150;
                    //Shopのカードを更新
                    void_leric[i] = true;
                    leric_alpha[i] = 0.8; // 売り切れ
                    if (getData().leric.getLeric().at(18))discount = 0.8;
                } else {
                }
                return;
            }
            if (leric_alpha[i] < 0.4 && isHovered_leric) {
                leric_alpha[i] += 0.1;
                if (leric_alpha[i] > 0.4) leric_alpha[i] = 0.4;
            } else if (leric_alpha[i] > 0.0 && !isHovered_leric) {
                leric_alpha[i] -= 0.1;
                if (leric_alpha[i] < 0.0) leric_alpha[i] = 0.0;
            }
        }
    }


    //戻るボタンの更新
    bool isHovered_return = RectF{ 1600, 800, 225, 225 }.mouseOver();
    if (isHovered_return)Cursor::RequestStyle(CursorStyle::Hand);
    if (MouseL.down() && isHovered_return) {
        changeScene(State::Map, 0.5s);
        return;
    }
    if (return_alpha < 0.4 && isHovered_return) {
        return_alpha += 0.1;
        if (return_alpha > 0.4) return_alpha = 0.4;
    } else if (return_alpha > 0.0 && !isHovered_return) {
        return_alpha -= 0.1;
        if (return_alpha < 0.0) return_alpha = 0.0;
    }
}

#define money_check(price) (getData().money >= discount * price ? Palette::White : Palette::Red)

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

        for (int i = 0; i < 4; i++) {
            getData().leric.drawOne(leric_index[i], 450 + 300 * i, 700);
            {
                const ScopedColorMul2D colorMul{ ColorF{ 1.0 - leric_alpha[i], 1.0 - leric_alpha[i], 1.0 - leric_alpha[i] } };
                double scale = 1.0 - ((leric_alpha[i] <= 0.4) ? (leric_alpha[i] * 0.05) : 0.0); // アルファ値に応じて拡大
                price_img.scaled(scale).drawAt(500 + 300 * i, 1000);
                fontBitMap(U"150G").drawAt(520 + 300 * i, 900, money_check(150));
            }
        }

        //戻るボタン
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - return_alpha, 1.0 - return_alpha, 1.0 - return_alpha } };
            double scale = 1.0 - ((return_alpha <= 0.4) ? (return_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            back_button_img.scaled(0.75 * scale).draw(1600, 800, ColorF{ 1.0, 1.0, 1.0 });
        }
    }
    // バナーの描画
    banner.draw(getData().money, getData().Layer, getData().leric);
}

void Shop::drawFadeIn(double t) const {
    // 背景の描画
    background_img.draw(0, 0);
    // フェードインの描画処理
    double time = Clamp(t, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    normal_1.Draw({ 500, 300 }, time * 1.5, Math::Pi * (1.0 - time), time);
    price_img.drawAt(500, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap((discount == 0.8) ? U"40G" : U"50G").drawAt(520, 550 + 50 * (1.0 - time), ColorF{ money_check(50), time });

    time = Clamp(t - 0.2, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    normal_2.Draw({ 800, 300 }, time * 1.5, Math::Pi * (1.0 - time), time);
    price_img.drawAt(800, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap((discount == 0.8) ? U"40G" : U"50G").drawAt(820, 550 + 50 * (1.0 - time), ColorF{ money_check(50), time });

    time = Clamp(t - 0.4, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    uncommon.Draw({ 1100, 300 }, time * 1.5, Math::Pi * (1.0 - time), time);
    price_img.drawAt(1100, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap((discount == 0.8) ? U"80G" : U"100G").drawAt(1120, 550 + 50 * (1.0 - time), ColorF{ money_check(100), time });

    time = Clamp(t - 0.6, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
    rare.Draw({ 1400, 300 }, time * 1.5, Math::Pi * (1.0 - time), time);
    price_img.drawAt(1400, 650 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
    fontBitMap((discount == 0.8) ? U"120G" : U"150G").drawAt(1420, 550 + 50 * (1.0 - time), ColorF{ money_check(150), time });

    for (int i = 0; i < 4; i++) {
        time = Clamp(t - 0.8 + 0.2 * i, 0.0, 0.4) * 2.5; // 0.4を1.0に変換するための係数
        getData().leric.drawOne(leric_index[i], 450 + 300 * i, 700, time, Math::Pi * (1.0 - time));
        price_img.drawAt(500 + 300 * i, 1000 + 50 * (1.0 - time), ColorF{ 1.0, 1.0, 1.0, time });
        fontBitMap((discount == 0.8) ? U"120G" : U"150G").drawAt(520 + 300 * i, 900 + 50 * (1.0 - time), ColorF{ money_check(150), time });
    }

    // 戻るボタンの描画
    back_button_img.scaled(0.75).draw(1600, 800 + 50 * (1.0 - t), ColorF{ 1.0, 1.0, 1.0, time });

    // バナーの描画
    banner.draw(getData().money, getData().Layer, getData().leric);
    if (t <= 0.5) {
        const double progress = EaseInOutExpo(t * 2.0);
        loading_icon.draw(1920 * Math::Lerp(0.0, 1.0, progress), 0);
    }
}
#undef money_check
