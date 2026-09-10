#pragma once
#include <cstdint>
#include<iostream>
#include"bitboards.h"
#include "board.hpp"
#include "magic.h"
#include <cstring>

extern Bitboard pawnAttacks[2][64];
extern Bitboard pawnSplatsForward[64];
extern Bitboard pawnSplatsBackward[64];
extern Bitboard whitePawnPassedMask[64];
extern Bitboard blackPawnPassedMask[64];
extern Bitboard adjacentFileMasks[8];
extern Bitboard knightAttacks[64];
extern Bitboard kingAttacks[64];
extern Bitboard bishopAttacksMask[64];
extern Bitboard rookAttacksMask[64];
extern Bitboard rookAttacks[64][4096];
extern int rookShifts[64];
extern int bishopShifts[64];
extern Bitboard bishopAttacks[64][512];
extern Bitboard line[64][64];
extern Bitboard between[64][64];
extern Bitboard fileMasks[8];
constexpr Bitboard rankMasks[8] = {
    0xFFULL,       0xFF00ULL,       0xFF0000ULL,       0xFF000000ULL,
    0xFF00000000ULL, 0xFF0000000000ULL, 0xFF000000000000ULL, 0xFF00000000000000ULL
};
void initLineBetween();
void initLeaperAttacks();
void initBishopReleventOccupancyMask();
void initRookReleventOccupancyMask();
void initSliderAttacks(bool isBishop);
void initAllSliderAttacks();
void initShifts();
void initSplats();
void initPassedPawnsMask();
void initFileMasks();

inline Bitboard northFill(Bitboard bb){
    bb |= (bb <<  8);
    bb |= (bb << 16);
    bb |= (bb << 32);
    return bb;
}
inline Bitboard southFill(Bitboard bb){
    bb |= (bb >>  8);
    bb |= (bb >> 16);
    bb |= (bb >> 32);
    return bb;  
}
inline Bitboard getRookAttacks(int sq,Bitboard occupancy){
    Bitboard index=magicIndex(occupancy,rookMagicNumbers[sq],64-rookShifts[sq],rookAttacksMask[sq]);
    return rookAttacks[sq][index];
}

inline Bitboard getBishopAttacks(int sq,Bitboard occupancy){
    Bitboard index=magicIndex(occupancy,bishopMagicNumbers[sq],64-bishopShifts[sq], bishopAttacksMask[sq]);
    return bishopAttacks[sq][index];
}
