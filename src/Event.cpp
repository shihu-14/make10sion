#include "Event.hpp"

#include <algorithm>

using namespace std;

Event::Event(const InitData& init)
	: IScene(init),
	m_background(U"../../image/haikei_sentou.png"),
	event(U"../../image/map_event.png"),
	font(FontMethod::MSDF, 80, Typeface::Bold),
	sentences{ U"2マス追加",
		U"攻撃ゾーンと防御ゾーンの\n境界線を一個上にずらす",
		U"攻撃ゾーンと防御ゾーンの\n境界線を一個下にずらす" },
	currentIndex(static_cast<size_t>(Random(0, static_cast<int32>(sentences.size() - 1))))
{
	const int32 remaining = static_cast<int32>(getData().board_progress.UnlockableCells().size());
	if ((currentIndex == 0) && (remaining == 0)) {
		currentIndex = static_cast<size_t>(Random(1, 2));
	}
	if (currentIndex == 0) {
		unlock_target = Min(2, remaining);
		unlock_board.BeginUnlockSelection(getData().board_progress);
	}
}

void Event::update()
{
	if (currentIndex == 0) {
		if (MouseL.down()) {
			const Point cell = unlock_board.GetBoardCellAt(Cursor::Pos());
			if (getData().board_progress.Unlock({ cell.x, cell.y })) {
				++unlocked_in_event;
				unlock_board.BeginUnlockSelection(getData().board_progress);
				if (unlocked_in_event >= unlock_target) {
					changeScene(State::Map, 0.5s);
				}
			}
		}
		return;
	}

	if (confirm_rect.mouseOver()) {
		Cursor::RequestStyle(CursorStyle::Hand);
	}
	if (!confirm_rect.leftClicked()) return;
	const int32 relic_index = (currentIndex == 1) ? 10 : 11;
	auto& relics = getData().leric.getLeric();
	if (relic_index < static_cast<int32>(relics.size())) {
		++relics[relic_index];
	}
	changeScene(State::Map, 0.5s);
}

void Event::draw() const
{
	m_background.scaled(0.5).draw();
	if (currentIndex == 0) {
		unlock_board.DrawBoard(1);
		font(U"解放するマスを選択 {}/{}"_fmt(unlocked_in_event, unlock_target))
			.drawAt(Scene::Center().x, 80, Palette::White);
		return;
	}

	event.scaled(2.0).drawAt(Scene::Center());
	font(sentences[currentIndex]).drawAt(Scene::Center().x, 350, Palette::Black);
	confirm_rect.draw(Palette::White).drawFrame(4, Palette::Black);
	font(U"決定").drawAt(confirm_rect.center(), Palette::Black);
}
