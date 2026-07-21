#include <cstdint>
#include<iostream>
#include<chrono>
#include"bitboards.h"
#include"board.h"
#include "makeMove.h"
#include "moves.h"
#include "perft.h"
#include "types.h"
#include<windows.h>
using namespace std;
int main(){
    Board board;
    


    // see exactly where each field sits / how much padding exists

    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes=perftRoot(board,7);
    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? (double)nodes / seconds : 0.0;
    cout<<"\n nps = "<<nps;
    cout<<"\n";
    // SetConsoleOutputCP(CP_UTF8);
    // SetConsoleCP(CP_UTF8);
    // board.print(board.occupied);
    return 0;
}