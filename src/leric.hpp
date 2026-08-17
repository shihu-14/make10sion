# ifndef LERIC_HPP
# define LERIC_HPP

# include <Siv3D.hpp>
# include <vector>

class Leric {
private:
	std::vector<Texture> leric_imgs;
	std::vector<int> leric_sum;
public:
	Leric();
	std::vector<int>& getLeric();
	const std::vector<int>& getLeric() const;
	void Reset();
	void draw() const;
	void drawOne(int index, int x, int y, double alpha = 1.0, double angle = 0.0) const;
};

# endif // LERIC_HPP
