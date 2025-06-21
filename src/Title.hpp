#ifndef Title_HPP
#define Title_HPP


class Title : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Title(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
}

#endif
