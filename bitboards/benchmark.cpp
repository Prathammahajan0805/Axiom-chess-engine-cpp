#include "board.h"
#include"perft.h"
#include <cstdint>
#include<chrono>
using namespace std;
void bench(Board& board){
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes=perftRoot(board, 7);
    auto end = std::chrono::high_resolution_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? (double)nodes / seconds : 0.0;
    cout<<"\n nps = "<<nps;
    cout<<"\nnodes : "<<nodes;
    cout<<"\n";
}