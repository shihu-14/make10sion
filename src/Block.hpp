#ifndef Block_HPP
#define Block_HPP
#include <string>
#include <vector>
#include <iterator>

struct Piece {
	char content;
	int x, y;
	int stat;
};


class Block {
	std::vector<std::vector<Piece>> contents;
	int sizeX, sizeY;
	int posX, posY;
	int stat;
	//画像読み込み
	std::vector<Texture> number_imgs;
	const Texture background_img{ U"../image/card_tile.png" };
	const Texture minus_img{ U"../image/minus.png" };
	const Texture plus_img{ U"../image/plus.png" };
	const Texture kakeru_img{ U"../image/kakeru.png" };
	const Texture waru_img{ U"../image/waru.png" };
	const Texture card_tile_img{ U"../image/card_tile.png" };
	const Texture left_img{ U"../image/card_sukima_left.png" };
	const Texture right_img{ U"../image/card_sukima_right.png" };
	const Texture top_img{ U"../image/card_sukima_top.png" };
	const Texture bottom_img{ U"../image/card_sukima_bottom.png" };
public:
	Block();
	std::pair<int, int> Size() const { return { sizeX, sizeY }; }
	std::pair<int, int> GetPos() const { return { posX, posY }; }
	void SetPos(int x, int y) { posX = x; posY = y; }
	Piece& GetPiece(int x, int y) { return contents[x][y]; }
	void Rotate();
	int GetStat() const { return stat; }
	void SetStat(int newStat) { stat = newStat; }
	bool IsDragging();
	bool IsHovered();
	void Draw(int x, int y, double size = 1.0, double angle = 0.0, double alpha = 1.0) const;

	Block& operator=(const Block& other);
	Block& operator=(const std::string& value);
};

#endif
