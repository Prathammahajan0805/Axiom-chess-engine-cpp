#include<iostream>
#include"bitboards.h"
#include"board.h"

int main(){
    Board board;
    std::cout << "White:\n";
        displayBitboard(board.whitePieces);

        std::cout << "Black:\n";
        displayBitboard(board.blackPieces);

        std::cout << "Occupied:\n";
        displayBitboard(board.occupied);
    return 0;
}