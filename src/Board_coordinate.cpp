#include <Siv3D.hpp>
#include "Board.hpp"
using namespace std;

//private
void Board::PutBlock(){
    //
}

void Board::TakeOutBlock(){
    //
}

//public
//選択されているBlockが渡される
void Board::PassBlock(const Block& selectedBlock){
    block = selectedBlock;
}