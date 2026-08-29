#pragma once
#include <cstdint>
#include"moves.h"
#include "types.h"
struct TTEntry{
    uint64_t key;
    Move bestMove;
    int depth;
    int score;
    TTFlag flag;
    bool isValid=false;
};

constexpr int TT_SIZE=1<<20;
extern TTEntry TT_Table[TT_SIZE];
TTEntry* probe(const uint64_t hash);
void store(uint64_t hash,Move bestMove,int depth,int score,TTFlag flag);
void clearTT();
