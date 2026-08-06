#include"TT.h"
#include <cstdint>
TTEntry TT_Table[TT_SIZE];

TTEntry* probe(const uint64_t hash){
    TTEntry &e= TT_Table[hash&(TT_SIZE-1)];
    if(e.isValid && e.key==hash) return &e;
    return nullptr;
}

void store(uint64_t hash, Move bestMove, int depth, int score, TTFlag flag){
    TTEntry &e=TT_Table[hash & (TT_SIZE-1)];
    if(e.isValid && e.depth>depth) return;
    e={hash,bestMove,depth,score,flag,true};
}