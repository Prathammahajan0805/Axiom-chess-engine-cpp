#include <cstdint>
#include<iostream>
#include<chrono>
#include "attacks.h"
#include"bitboards.h"
#include"board.h"
#include "makeMove.h"
#include "moves.h"
#include "perft.h"
#include "types.h"
#include"search.h"
#include<windows.h>
using namespace std;
int main(){
    Board board;
    //board.loadFEN("8/3k4/8/8/8/2K3Q1/8/8 b - - 0 1");
    // SetConsoleOutputCP(CP_UTF8);
    // SetConsoleCP(CP_UTF8);
    // int depth = 5;          // bump once you have alpha-beta
    // int maxMoves = 200;      // safety cap so it can't loop forever
 
    // for(int moveNum = 0; moveNum < maxMoves; moveNum++){
    //     board.print(board.occupied);
 
    //     MoveList legal = generateMoves(board);
    //     if(legal.count == 0){
    //         int sq = board.sideToMove==WHITE? board.whiteKingSquare : board.blackKingSquare;
    //         bool inCheck = isSquareAttacked(sq, (Color)(board.sideToMove^1), board.occupied, board);
    //         cout << (inCheck ? "Checkmate! " : "Stalemate! ")
    //              << (board.sideToMove==WHITE ? "Black" : "White") << " wins/draws.\n";
    //         break;
    //     }
 
    //     Move mv = findBestMove(board, depth);
    //     cout << (board.sideToMove==WHITE ? "White" : "Black")
    //          << " plays: " << printMove(mv) << "\n";
    //     makeMove(board, mv);
    // }
    
    

    // see exactly where each field sits / how much padding exists

    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes=perftRoot(board, 7);
    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? (double)nodes / seconds : 0.0;
    cout<<"\n nps = "<<nps;
    cout<<"\nnodes : "<<nodes;
    cout<<"\n";
    
    // board.print(board.occupied);
    return 0;
}