#ifndef Deck_HPP
#define Deck_HPP
#include <Siv3D.hpp> // Siv3D v0.6.16
#include "common.hpp"
#include <vector>

class Deck : public App::Scene{
private:
	std::vector<std::pair<int,int>> card_pos; //カードの位置
public:
	Deck(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
};

#endif
