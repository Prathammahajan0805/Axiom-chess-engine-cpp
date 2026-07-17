#pragma once
#include <cstdint>
#include<iostream>
#include"bitboards.h"
#include "board.h"
#include "magic.h"
#include <cstring>

extern Bitboard pawnAttacks[2][64];
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

void initLineBetween();
void initLeaperAttacks();
void initBishopReleventOccupancyMask();
void initRookReleventOccupancyMask();
void initSliderAttacks(bool isBishop);
void initAllSliderAttacks();
void initShifts();

inline Bitboard getRookAttacks(int sq,Bitboard occupancy){
    Bitboard index=magicIndex(occupancy,rookMagicNumbers[sq],64-rookShifts[sq],rookAttacksMask[sq]);
    return rookAttacks[sq][index];
}

inline Bitboard getBishopAttacks(int sq,Bitboard occupancy){
    Bitboard index=magicIndex(occupancy,bishopMagicNumbers[sq],64-bishopShifts[sq], bishopAttacksMask[sq]);
    return bishopAttacks[sq][index];
}
