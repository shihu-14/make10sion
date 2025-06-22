#include "Event.hpp"
using namespace std;

Event::Event(const InitData& init) : IScene(init),
m_background(U"../../image/haikei_sentou.png"), // 背景画像のパスを指定
event(U"map_event.png") // イベント画像のパスを指定
	
{
	std::vector<String> sentences =
	{
		U"マス２追加",
		U"攻撃ゾーンと防御ゾーンの境界線を一個上にずらす",
		U"攻撃ゾーンと防御ゾーンの境界線を一個下にずらす"
	};
	size_t currentIndex = Random(0, static_cast<int>(sentences.size() - 1));
	// size_t型とint型の混在に注意。Randomの引数はint型なのでキャストしています。
	

}

void Event::update() {
	
}

void Event::draw() const {
	m_background.scaled(0.5).draw(); // 背景画像を描画
	event.drawAt(Scene::Center()); // イベント画像を画面中央に描画
	font((U"sentences[currentIndex]).drawAt(400, 300, Palette::Black); // 画面中央に文章を描画"));
}
