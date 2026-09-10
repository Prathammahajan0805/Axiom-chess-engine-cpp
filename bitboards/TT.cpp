#include "TT.h"

#include <cstdint>
#include <cstring>
#include <algorithm>

TTEntry* TT_Table = nullptr;

uint64_t TT_SIZE = 0;

void resizeTT(int mb)
{
    // UCI limits
    mb = std::clamp(mb, 1, 4096);

    uint64_t bytes =
        static_cast<uint64_t>(mb) * 1024ULL * 1024ULL;
    uint64_t entries =
        bytes / sizeof(TTEntry);

    if (entries == 0)
        entries = 1;
    uint64_t size = 1;

    while ((size << 1) <= entries)
        size <<= 1;

    delete[] TT_Table;

    // Allocate new TT
    TT_Table = new TTEntry[size];

    TT_SIZE = size;

    clearTT();
}


TTEntry* probe(const uint64_t hash)
{
    if (TT_Table == nullptr)
        return nullptr;

    TTEntry &e =
        TT_Table[hash & (TT_SIZE - 1)];

    if (e.isValid && e.key == hash)
        return &e;

    return nullptr;
}


void store(
    uint64_t hash,
    Move bestMove,
    int depth,
    int score,
    TTFlag flag
)
{
    if (TT_Table == nullptr)
        return;

    TTEntry &e =
        TT_Table[hash & (TT_SIZE - 1)];

    // Don't replace a deeper entry
    if (e.isValid && e.depth > depth)
        return;

    e.key = hash;
    e.bestMove = bestMove;
    e.depth = depth;
    e.score = score;
    e.flag = flag;
    e.isValid = true;
}


void clearTT()
{
    if (TT_Table == nullptr)
        return;

    std::memset(
        TT_Table,
        0,
        TT_SIZE * sizeof(TTEntry)
    );
}