#include "Leric.hpp"
using namespace std;

Leric::Leric() :leric_imgs(19), leric_sum(19, 0) {
	// 画像の読み込み
	for (int i = 0; i < 19; i++)
		leric_imgs.at(i) = Texture{ Unicode::Widen("../../image/leric_" + to_string(i + 1) + ".png") };
}

vector<int>& Leric::getLeric() {
	return leric_sum;
}
void Leric::draw() const {
	// 画像を描画
	for (size_t i = 0; i < leric_imgs.size(); i++) {
		if (leric_sum.at(i) == 0) continue; // 0の時は描画しない
		for (int j = 0; j < leric_sum.at(i); j++)
			leric_imgs.at(i).scaled(0.5).draw(510 + i * 50, 50); // 適当な位置に描画
	}
}

void Leric::drawOne(int index, int x, int y, double alpha, double angle) const {
	if (index < 0 || index >= static_cast<int>(leric_imgs.size())) return; // 範囲外のインデックスは無視
	leric_imgs.at(index).scaled(1.0).rotated(angle).draw(x, y, ColorF{ 1.0, 1.0, 1.0, alpha });
}