# ifndef LERIC_HPP
# define LERIC_HPP

# include <Siv3D.hpp>
# include <vector>
# include "Block.hpp"
# include "Board.hpp"

class Leric {
private:
	std::vector<Texture> leric_imgs;
	std::vector<int> leric_sum;
public:
	Leric();
	std::vector<int> &getLeric();
	void draw() const;
};

# endif // LERIC_HPP