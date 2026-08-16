#include "Result.hpp"
using namespace std;

Result::Result(const InitData& init) : IScene(init),
retry_rect(Arg::center = Vec2(Scene::Center().x - 450, Scene::Height() - 200), 300, 300, 20),
// 左下寄りボタン
title_rect(Arg::center = Vec2(Scene::Center().x +450, Scene::Height() - 200), 300, 300, 20)
// 右下寄りボタン
{
	Scene::SetBackground(Palette::Skyblue);
	title_back = Texture(U"../../image/bottun_titlehe.png");
	retry = Texture(U"../../image/bottun_mouitido.png");
    
    score = getData().Layer * 5+ getData().enemy*2+ getData().Layer/10*50;
}



void Result::update(){
    
    if (retry_rect.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
        if (retry_rect.leftClicked()) { // マウス左ボタンがクリックされた瞬間
			getData().ResetForNewRun();
			changeScene(State::Battle, 0.5s);
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
    title_back.drawAt(title_rect.center());
	retry.drawAt(retry_rect.center());
    if (getData().run_outcome == GameStateRules::RunOutcome::Clear)
    {
        font(U"Clear!\nScore:{}\n"_fmt(score)).drawAt(Scene::Center().x, Scene::Center().y - 100, ColorF{ 0.2 });
    }
    else
    {
        font(U"Game Over\nScore:{}\n"_fmt(score)).drawAt(Scene::Center().x, Scene::Center().y - 100, ColorF{ 0.2 });
    }
	
}
