#include "Event.hpp"
using namespace std;

Event::Event(const InitData& init) : IScene(init),
m_background(U"../../image/haikei_sentou.png"), // 背景画像のパスを指定
event(U"../../image/map_event.png"),// イベント画像のパスを指定
sentences{ U"2マス追加",
	U"攻撃ゾーンと防御ゾーンの \n 境界線を一個上にずらす",
	U"攻撃ゾーンと防御ゾーンの \n 境界線を一個下にずらす" },
	currentIndex(Random(0, static_cast<int>(sentences.size() - 1))),
	font(FontMethod::MSDF, 80, Typeface::Bold)
{

}

void Event::update() {

}

void Event::draw() const {
	m_background.scaled(0.5).draw(); // 背景画像を描画
	event.scaled(2.0).drawAt(Scene::Center()); // イベント画像を画面中央に描画
	font(sentences[currentIndex]).draw(400, 300, Palette::Black); // 画面中央に文章を描画"));
}
