#include"magic.h"
#include "bitboards.h"
#include <cstdint>

uint64_t magicIndex(Bitboard occupied, Bitboard m,int shift,Bitboard mask){
    Bitboard blockers = occupied & mask;
    return (blockers * m) >> shift;
}