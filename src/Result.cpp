#include "Result.hpp"
using namespace std;

Result::Result(const InitData& init) : IScene(init),
retry_rect(Arg::center = Vec2(Scene::Center().x - 400, Scene::Height() - 450), 420, 100, 20),
// 左下寄りボタン
title_rect(Arg::center = Vec2(Scene::Center().x +450, Scene::Height() - 300), 200, 200, 20)
// 右下寄りボタン
{
	Scene::SetBackground(Palette::White);
	title_back = Texture(U"../../image/title_end_0.png");
	retry = Texture(U"../../image/back_button_deck0.png");
    
    score = getData().Layer * 5;
}



void Result::update(){
    if (getData().Layer == 30)
    {
        font(U"Clear!\n \n Score:{}"_fmt(score)).drawAt(200, Vec2{ Scene::Center().x,Scene::Center().y }, ColorF{ 0.2 });
    }else{
		font(U"Game Over\n \n Score:{}"_fmt(score)).drawAt(200, Vec2{ Scene::Center().x,Scene::Center().y }, ColorF{ 0.2 });
	}
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
    title_back.draw(title_rect.center());
	retry.scaled(0.5).draw(retry_rect.center());
	
}

