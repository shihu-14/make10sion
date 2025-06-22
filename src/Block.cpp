#include <Siv3D.hpp> // Siv3D v0.6.16
#include "Block.hpp"  
using namespace std;

Block::Block() : sizeX(0), sizeY(0), stat(0), number_imgs(8), special_imgs(17), posX(0), posY(0) {
	for (int i = 0; i < 8; i++)
		number_imgs.at(i) = Texture{ Unicode::Widen("../../image/number_" + to_string(i) + ".png") };
	for (int i = 0; i < 17; i++) {
		string alphabet(1, 'a' + i);
		special_imgs.at(i) = Texture{ Unicode::Widen("../../image/special_" + alphabet + ".png") };
	}
}

Block::Block(const string& value) : sizeX(0), sizeY(0), posX(0), posY(0), stat(0), number_imgs(8), special_imgs(17) {
	for (int i = 0; i < 8; i++)
		number_imgs.at(i) = Texture{ Unicode::Widen("../../image/number_" + to_string(i) + ".png") };
	for (int i = 0; i < 17; i++) {
		string alphabet(1, 'a' + i);
		special_imgs.at(i) = Texture{ Unicode::Widen("../../image/special_" + alphabet + ".png") };
	}
	*this = value; // コンストラクタで文字列をセット
}

Block& Block::operator=(const Block& other) {
	if (this == &other) return *this; // 自分自身への代入を防ぐ
	sizeX = other.sizeX;
	sizeY = other.sizeY;
	posX = other.posX;
	posY = other.posY;
	stat = other.stat;
	contents = other.contents;
	return *this;
}

Block& Block::operator=(const string& value) {
	contents.clear();
	// stringstream を使って文字列を行ごとに安全に処理します
	std::stringstream ss(value);
	std::string line;
	std::vector<std::string> lines;
	sizeX = 0;
	sizeY = 0;
	// 全ての行を読み込み、最大の行の長さと行数を取得します
	while (std::getline(ss, line)) {
		sizeY++;
		if (sizeX == 0)sizeX = (int)(line.length());
		lines.push_back(line);
	}

	if (sizeY == 0)
		return *this; // 空の文字列の場合は何もしない
	int centerX = (sizeX / 2) * 50 + (sizeX % 2) * 25; // 奇数の場合は中央を調整
	int centerY = (sizeY / 2) * 50 + (sizeY % 2) * 25;
	contents.resize(sizeX, vector<Piece>(sizeY));
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			Piece p;
			p.content = lines.at(y)[x];
			p.x = x * 50 + 25 - centerX; // ピースのx座標を計算
			p.y = y * 50 + 25 - centerY; // ピースのy座標を計算
			p.stat = 0;
			contents.at(x).at(y) = p;
		}
	}
	return *this;
}

bool Block::operator==(const Block& other) const {
	if (sizeX != other.sizeX || sizeY != other.sizeY) return false;
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			if (contents[x][y].content != other.contents[x][y].content) return false;
		}
	}
	return true;
}

void Block::Rotate() {
	vector<vector<Piece>> newContents(sizeX, vector<Piece>(sizeY));
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			newContents[x][sizeY - 1 - y] = contents[y][x];
			newContents[x][sizeY - 1 - y].x = contents[y][x].y;
			newContents[x][sizeY - 1 - y].y = -contents[y][x].x;
		}
	}
	contents = move(newContents);
	swap(sizeX, sizeY);
}

bool Block::IsDragging() {
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			RectF rect{ Arg::center(contents[x][y].x + posX, contents[x][y].y + posY), 50, 50 };
			if (rect.mouseOver() && MouseL.pressed())return true;
		}
	}
	return false;
}

bool Block::IsHovered() {
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			RectF rect{ Arg::center(contents[x][y].x + posX, contents[x][y].y + posY), 50, 50 };
			if (rect.mouseOver())return true;
		}
	}
	return false;
}

void Block::Draw(pair<int, int> pos, double size, double angle, double alpha) const {
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			const Piece& p = contents[x][y];
			if (p.content == '$') continue;
			Texture img;
			bool mode_alpha = false;
			if (p.content == '+') img = plus_img;
			else if (p.content == '-') img = minus_img;
			else if (p.content == '*') img = kakeru_img;
			else if (p.content == '/') img = waru_img;
			else if (isdigit(p.content)) img = number_imgs[p.content - '0'];
			else if ('a' <= p.content && p.content <= 'q') img = special_imgs[p.content - 'a'];
			else if ('A' <= p.content && p.content <= 'H') {
				mode_alpha = true;
				img = number_imgs[p.content - 'A'];
			} else continue; // 不明な文字はスキップ
			//Blockの中心座標と各ピースの座標との距離を計算
			double distance = sqrt(p.x * p.x + p.y * p.y) * size; // ピースの座標を基準に距離を計算
			double draw_angle = atan2(p.y, p.x) + angle; // 回転角度を加える
			double draw_x = (double)pos.first + cos(draw_angle) * distance;
			double draw_y = (double)pos.second + sin(draw_angle) * distance;

			card_tile_img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha * (mode_alpha ? 0.3 : 1.0) });
			img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha * (mode_alpha ? 0.3 : 1.0) });
			// 境界を描画
			if ((x == 0) || (x > 0 && contents[x - 1][y].content == '$')) {
				left_img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha });
			}
			if ((x == sizeX - 1) || (x < sizeX - 1 && contents[x + 1][y].content == '$')) {
				right_img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha });
			}
			if ((y == 0) || (y > 0 && contents[x][y - 1].content == '$')) {
				top_img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha });
			}
			if ((y == sizeY - 1) || ((y < sizeY - 1) && contents[x][y + 1].content == '$')) {
				bottom_img.scaled(size).rotated(angle).drawAt(draw_x, draw_y, ColorF{ 1.0, 1.0, 1.0, alpha });
			}
		}
	}
}
