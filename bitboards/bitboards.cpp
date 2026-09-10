#include "bitboards.h"
#include "board.hpp"
#include <iostream>
#include<string>
using namespace std;
std::string pieceToUnicode(Piece piece) {
  switch (piece) {
      case WP: return "♙";
      case WN: return "♘";
      case WB: return "♗";
      case WR: return "♖";
      case WQ: return "♕";
      case WK: return "♔";

      case BP: return "♟";
      case BN: return "♞";
      case BB: return "♝";
      case BR: return "♜";
      case BQ: return "♛";
      case BK: return "♚";

      default: return " ";
  }
}

char pieceToChar(Piece piece){
    switch(piece){
        case WP: return 'P';break;
        case WB: return 'B';break;
        case WN: return 'N';break;
        case WR: return 'R';break;
        case WQ: return 'Q';break;
        case WK: return 'K';break;
        case BP: return 'p';break;
        case BB: return 'b';break;
        case BN: return 'n';break;
        case BR: return 'r';break;
        case BQ: return 'q';break;
        case BK: return 'k';break;
        case NO_PIECE: return '\0';break;
    }
    return '\0';
}

void printBitboard(Bitboard bb){
    string s="\n  --------------------------------\n";
    for (int rank = RANKV_8; rank >= RANKV_1; rank--) {
        for (int file = FILEV_A; file <= FILEV_H; file++) {
            if(file==FILEV_A) s+=to_string(rank+1)+" ";
            if(getBit(bb, getSquare(file, rank))) s+= "| X ";
            else s+="|   ";
        }
        s+="| \n  --------------------------------\n";
        
    }
    s+="    a   b   c   d   e   f   g   h \n";
    cout<<s;
}
    