#include "Result.hpp"
using namespace std;

Result::Result(const InitData& init) : IScene(init){
	Scene::SetBackground(Palette::White);
	retry_rect = Rect{ 100, 400, 200, 100 };
	title_rect = Rect{ 100, 600, 200, 100 }; 
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

            System::Exit();
        }
    }
}

void Result::draw() const{
	
}

