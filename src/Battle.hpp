#ifndef Battle_HPP
#define Battle_HPP


class Battle : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Battle(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
}

#endif
