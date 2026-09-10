#pragma once

#include <cstdint>
#include "moves.h"
#include "types.h"

struct TTEntry {
    uint64_t key = 0;
    Move bestMove{};
    int depth = 0;
    int score = 0;
    TTFlag flag{};
    bool isValid = false;
};
extern uint64_t TT_SIZE;

extern TTEntry* TT_Table;

TTEntry* probe(uint64_t hash);

void store(
    uint64_t hash,
    Move bestMove,
    int depth,
    int score,
    TTFlag flag
);

void clearTT();

void resizeTT(int mb);