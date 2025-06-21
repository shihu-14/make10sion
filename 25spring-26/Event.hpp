#ifndef Event_HPP
#define Event_HPP
#include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include "common.hpp"

class Event : public App::Scene {
	private:
	//Write private functions or variables here.
		Texture m_background;  // 背景画像用のTextureオブジェクト
public:
	Event(const InitData& init);
	//Write public functions here.
	void update() override;
	void draw() const override;
};

