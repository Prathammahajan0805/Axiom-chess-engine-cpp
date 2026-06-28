#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include "board.h"
#include "move.h"
#include "piece.h"
using namespace std;
int main() {
    Board board;
    board.display();
    string italianFEN = "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R";
    board.loadFEN(italianFEN);
    board.display();
    cout << "FEN: " << board.toFEN() << "\n";
    
    return 0;
}