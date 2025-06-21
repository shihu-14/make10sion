#ifndef Result_HPP
#define Result_HPP


class Result : public App::Scene{
private:
	//Write private functions or varables here.
	
public:
	Result(const InitData& init);
	//Write public functions here.
	
	void update() override;
	void draw() const override;
};

#endif
