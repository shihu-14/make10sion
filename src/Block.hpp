#ifndef Block_HPP
#define Block_HPP
#include <string>
#include <vector>
#include <iterator>

struct Piece {
	std::string content;
	int x, y;
	int stat;
};


class Block {
	std::vector<std::vector<Piece>> contents;
	int sizeX, sizeY;
	int stat;
	std::vector<Texture> number_imgs;
	//画像読み込み
	const Texture background_img{ U"../image/card_tile.png" };
	const Texture minus_img{ U"../image/minus.png" };
	const Texture plus_img{ U"../image/plus.png" };
	const Texture kakeru_img{ U"../image/kakeru.png" };
	const Texture waru_img{ U"../image/waru.png" };
	const Texture left_img{ U"../image/card_sukima_left.png" };
public:
	Block();
	std::pair<int, int> Size() const { return { sizeX, sizeY }; }
	Piece& GetPiece(int x, int y) { return contents[x][y]; }
	void Rotate();
	int GetStat() const { return stat; }
	void SetStat(int newStat) { stat = newStat; }

	void Draw() const;

	Block& operator=(const std::string& value);
};

#endif
