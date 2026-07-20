#include<iostream>
#include<chrono>
#include"bitboards.h"
#include"board.h"
#include "perft.h"
using namespace std;
int main(){
    Board board;
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = perft(board, 6);
    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? (double)nodes / seconds : 0.0;
    cout<<"nodes = "<<nodes<<"\n";
    cout<<"nps = "<<nps;
    cout<<"\n seconds = "<<seconds;

    return 0;
}