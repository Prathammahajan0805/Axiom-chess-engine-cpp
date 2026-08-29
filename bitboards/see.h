// see.h
#pragma once
#include "types.h"
#include "board.h"
int see(const Board& board, const Move& move);
inline bool seeGE(const Board& board, const Move& move, int threshold){
    return see(board,move)>=threshold;
}