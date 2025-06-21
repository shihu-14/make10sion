#ifndef Deck_HPP
#define Deck_HPP


class Deck : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Deck(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
};

#endif
