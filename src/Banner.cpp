#include "Banner.hpp"
using namespace std;

bool Banner::update(vector<Block>& deck_data, bool allow_deck_open) {
    return update(deck_data, allow_deck_open, Cursor::Pos(), MouseL.down(), MouseL.up(), Window::GetState().focused);
}

bool Banner::update(vector<Block>& deck_data, bool allow_deck_open, Point cursor_pos,
    bool left_down, bool left_up, bool focused) {
    isHovered_setting = IsSettingButtonHovered(cursor_pos);
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
    isHovered_deck = IsDeckButtonHovered(cursor_pos);
    Cursor::RequestStyle(isHovered_deck ? CursorStyle::Hand : CursorStyle::Default);
    if (!focused || !allow_deck_open) deck_button_armed = false;
    if (focused && allow_deck_open && left_down && isHovered_deck) deck_button_armed = true;
    if (left_up && deck_button_armed && allow_deck_open && isHovered_deck) {
        deck.init(deck_data); // デッキの初期化
        deck_mode = true; // デッキモードに入る
        deck_alpha = 0.0; // デッキボタンのアルファ値をリセット
        deck_button_armed = false;
    } else if (deck_alpha < 0.4 && isHovered_deck) {
        deck_alpha += 0.1;
        if (deck_alpha > 0.4) deck_alpha = 0.4;
    } else if (deck_alpha > 0.0 && !isHovered_deck) {
        deck_alpha -= 0.1;
        if (deck_alpha < 0.0) deck_alpha = 0.0;
    }
    if (left_up) deck_button_armed = false;
    return false; // デッキモードに入っていない場合はfalseを返す
}

bool Banner::IsDeckButtonHovered(Point cursor_pos) const {
    return RectF{ 1520, 0, 150, 150 }.contains(cursor_pos);
}

bool Banner::IsSettingButtonHovered(Point cursor_pos) const {
    return RectF{ 1720, 0, 150, 150 }.contains(cursor_pos);
}

void Banner::CancelPointerGesture() {
    deck_button_armed = false;
}

void Banner::draw(const int money, const int floor, const Leric& leric) const {
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
