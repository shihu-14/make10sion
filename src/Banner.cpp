#include "Banner.hpp"
using namespace std;

void Banner::init(int global_money, int global_floor) {
    money = global_money;
    floor = global_floor;
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
    money_img.draw(500, 20);
    if (!deck_mode) {
        deck_img.scaled(0.5).draw(1520, 0);
        RectF{ 1520, 0, 150, 150 }.draw(ColorF{ 0.0, 0.0, 0.0, deck_alpha });
    }
    setting_img.scaled(0.5).draw(1720, 0, ColorF{ 1.0, 1.0 });
    RectF{ 1720, 0, 150, 150 }.draw(ColorF{ 0.0, 0.0, 0.0, setting_alpha });
    fontBitMap(money).draw(610, 35, ColorF{ 1.0, 1.0, 1.0 });
    fontBitMap2(floor).drawAt(65, 65, ColorF{ 0.0, 0.0, 0.0 });
}

