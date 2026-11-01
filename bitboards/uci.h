#pragma once
#include<iostream>
#include"board.hpp"
struct GoParams {
    int depth       = 0;
    int64_t movetime = -1;
    int64_t wtime = -1, btime = -1, winc = 0, binc = 0;
    int movestogo   = -1;
    bool infinite   = false;
    bool perft=false;
};
extern bool ponder;
extern GoParams p;
std::string moveToUci(const Move& move);
void initUCI();