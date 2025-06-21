#ifndef Map_HPP
#define Map_HPP


class Map : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Map(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
};

#endif
