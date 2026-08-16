#ifndef Event_HPP
#define Event_HPP

#include <Siv3D.hpp>
#include "Board.hpp"
#include "common.hpp"

class Event : public App::Scene {
private:
	Texture m_background;
	Texture event;
	const Font font;
	std::vector<String> sentences;
	size_t currentIndex;
	Board unlock_board;
	int32 unlock_target = 0;
	int32 unlocked_in_event = 0;
	RoundRect confirm_rect{ Arg::center = Vec2{ 960, 820 }, 360, 110, 20 };

public:
	Event(const InitData& init);
	void update() override;
	void draw() const override;
};

#endif
