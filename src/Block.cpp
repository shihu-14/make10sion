#include <Siv3D.hpp> // Siv3D v0.6.16
#include "Block.hpp"  
using namespace std;

Block::Block() : sizeX(0), sizeY(0), stat(0), number_imgs(8) {
	for (int i = 0; i < 8; i++)
		number_imgs.at(i) = Texture{ Unicode::Widen("../image/number_" + to_string(i) + ".png") };
}

Block& Block::operator=(const string& value) {
	sizeY = count(value.begin(), value.end(), '\n') + 1;
	auto firstNewline = value.find('\n');
	sizeX = firstNewline != string::npos ? firstNewline : value.size();
	int centerX = (sizeX / 2) * 50;
	int centerY = (sizeY / 2) * 50;
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			Piece p;
			p.content = value.at(x);
			p.x = centerX + (x - sizeX / 2) * 50;
			p.y = centerY + (y - sizeY / 2) * 50;
			p.stat = 0;
			contents.at(y).push_back(p);
		}
	}
}

void Block::Rotate() {
	vector<vector<Piece>> newContents(sizeX, vector<Piece>(sizeY));
	for (int y = 0; y < sizeY; y++) {
		for (int x = 0; x < sizeX; x++) {
			newContents[x][sizeY - 1 - y] = contents[y][x];
		}
	}
	contents = move(newContents);
	swap(sizeX, sizeY);
}

void Block::Draw() const {

}
