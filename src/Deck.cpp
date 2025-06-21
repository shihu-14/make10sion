#include "Deck.hpp"
#include "common.hpp"
using namespace std;

Deck::Deck(const InitData& init) : IScene(init) {
	//test
	Block block;
	block = "2\n3";
	getData().Deck.push_back(block);
	block = "$42\n3+$";
	getData().Deck.push_back(block);
	for (int i = 0;i < getData().Deck.size();i++)
		getData().Deck.at(i).SetPos( 100 + (i % 9) * 200, 200 + 250* (i / 9) ); //カードの位置を設定
}



void Deck::update() {

}

void Deck::draw() const {
	for (int i = 0;i < getData().Deck.size();i++) 
		getData().Deck.at(i).Draw(0, 0, 1.0, 0.0, 1.0);
}

