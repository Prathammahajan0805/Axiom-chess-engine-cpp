#include "bitboards.h"
#include <iostream>
#include<string>
using namespace std;
void displayBitboard(Bitboard bb) {
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
string squareToString(int sq){
  string s="";
  int rank=sq/8;
  int file=sq%8;
  s+=static_cast<char>(file+'a');
  s+=to_string(rank+1);
  return s;
}
