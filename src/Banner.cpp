#include "Banner.hpp"
using namespace std;

void Banner::init(int global_money, int global_floor, Leric& global_leric) {
    money = global_money;
    floor = global_floor;
    leric = global_leric; // レリックの初期化
}

bool Banner::update(vector<Block>& deck_data) {
    isHovered_setting = RectF{ 1720, 0, 150, 150 }.mouseOver();
    //設定はただの飾り
    if (setting_alpha < 0.4 && isHovered_setting) {
        setting_alpha += 0.1;
        if (setting_alpha > 0.4) setting_alpha = 0.4;
    } else if (setting_alpha > 0.0 && !isHovered_setting) {
        setting_alpha -= 0.1;
        if (setting_alpha < 0.0) setting_alpha = 0.0;
    }

    if (deck_mode) {
        deck_mode = deck.update();
        return true; // デッキモードに入っている場合はtrueを返す
    }
    isHovered_deck = RectF{ 1520, 0, 150, 150 }.mouseOver();
    Cursor::RequestStyle(isHovered_deck ? CursorStyle::Hand : CursorStyle::Default);
    if (isHovered_deck && MouseL.up()) {
        deck.init(deck_data); // デッキの初期化
        deck_mode = true; // デッキモードに入る
        deck_alpha = 0.0; // デッキボタンのアルファ値をリセット
    } else if (deck_alpha < 0.4 && isHovered_deck) {
        deck_alpha += 0.1;
        if (deck_alpha > 0.4) deck_alpha = 0.4;
    } else if (deck_alpha > 0.0 && !isHovered_deck) {
        deck_alpha -= 0.1;
        if (deck_alpha < 0.0) deck_alpha = 0.0;
    }
    return false; // デッキモードに入っていない場合はfalseを返す
}

void Banner::draw() const {
    if (deck_mode)
        deck.draw(); // デッキの描画
    banner_img.draw(0, 0);
    floor_img.scaled(0.5).draw(0, 0);
    money_img.draw(200, 20);
    if (!deck_mode) {
        {
            const ScopedColorMul2D colorMul{ ColorF{ 1.0 - deck_alpha, 1.0 - deck_alpha, 1.0 - deck_alpha } };
            double scale = 1.0 - ((deck_alpha <= 0.4) ? (deck_alpha * 0.05) : 0.0); // アルファ値に応じて拡大
            deck_img.scaled(0.5 * scale).draw(1520, 0);
        }
    }
    setting_img.scaled(0.5).draw(1720, 0, ColorF{ 1.0, 1.0 });
    RectF{ 1720, 0, 150, 150 }.draw(ColorF{ 0.0, 0.0, 0.0, setting_alpha });
    fontBitMap(money).draw(310, 35, ColorF{ 1.0, 1.0, 1.0 });
    fontBitMap2(floor).drawAt(65, 65, ColorF{ 0.0, 0.0, 0.0 });
    leric.draw(); // レリックの描画
}

