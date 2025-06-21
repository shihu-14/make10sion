#include "Result.hpp"
using namespace std;

Result::Result(const InitData& init) : IScene(init){
	Scene::SetBackground(Palette::White);
	retry_rect = RectF{ 100, 400, 200, 100 };
}



void Result::update(){
	font(U"Clear!\n \n Score:{}").drawAt(200, Vec2{ Scene::Center().x,Scene::Center().y}, ColorF{0.2});

}

void Result::draw() const{
	
}

