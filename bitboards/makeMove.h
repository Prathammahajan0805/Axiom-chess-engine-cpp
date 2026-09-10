#pragma once

#include<vector>
#include "movegen.h"
#include"moves.h"
#include "bitboards.h"
#include "board.hpp"


inline bool pinAllows(int pinnedSq,const checkInfo &info,int to){
    return !(info.pinned & (1ULL<<pinnedSq)) || (info.pinRay[pinnedSq] & (1ULL<<to));
}
void makeMove(Board& board,Move& move);
void unMakeMove(Board& board);
void makeNullMove(Board& board);
void unmakeNullMove(Board& board);