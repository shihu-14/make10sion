#include "Result.hpp"
using namespace std;

Result::Result(const InitData& init) : IScene(init),
retry_rect(Arg::center = Vec2(Scene::Center().x + 20, Scene::Height() - 450), 420, 100, 20),
// 左下寄りボタン
title_rect(Arg::center = Vec2(Scene::Center().x + 20, Scene::Height() - 450), 420, 100, 20)
// 右下寄りボタン
{
	Scene::SetBackground(Palette::White);
	title_back = Texture(U"../../image/title_end_0.png");
	retry = Texture(U"../../image/back_button_deck0.png");
    
    score = getData().Layer * 5;
}



void Result::update(){
	font(U"Clear!\n \n Score:{}"_fmt(score)).drawAt(200, Vec2{ Scene::Center().x,Scene::Center().y}, ColorF{0.2});
    if (retry_rect.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
        if (retry_rect.leftClicked()) { // マウス左ボタンがクリックされた瞬間
            // 
            changeScene(State::Map, 0.5s);
        }
    }
    if (title_rect.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
        if (title_rect.leftClicked()) { // マウス左ボタンがクリックされた瞬間

            changeScene(State::Title, 0.5s);
        }
    }
}

void Result::draw() const{
    title_back.draw(300, 700);
	retry.scaled(0.5).draw(1500,700);
	
}

