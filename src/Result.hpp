#ifndef Result_HPP
#define Result_HPP
#include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include "common.hpp"


class Result : public App::Scene{
private:
	//Write private functions or varables here.
	const Font font{ FontMethod::MSDF, 80,Typeface::Bold };
	Texture title_back;
	Texture retry;
	RoundRect retry_rect;
	RoundRect title_rect;
	int score;
public:
	Result(const InitData& init);
	//Write public functions here.
	~Result() = default;

	void update() override;
	void draw() const override;
};

#endif
