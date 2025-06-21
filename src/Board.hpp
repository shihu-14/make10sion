#ifndef Board_HPP
#define Board_HPP


class Board : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Board(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
}

#endif
