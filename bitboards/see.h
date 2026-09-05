// see.h
#pragma once
#include "types.h"
#include "board.h"
static constexpr int SEE_VALUE[]{
    100,
    320,
    330,
    500,
    900,
    0,
    0
};
int see(const Board& board, const Move& move);
inline bool seeGE(const Board& board, const Move& move, int threshold){
    return see(board,move)>=threshold;
}