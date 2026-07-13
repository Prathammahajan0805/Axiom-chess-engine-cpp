#pragma once
#include <cstdint>
#include<iostream>
#include"bitboards.h"
#include "board.h"

extern Bitboard pawnAttacks[2][64];
extern Bitboard knightAttacks[64];
extern Bitboard kingAttacks[64];
extern Bitboard bishopAttacksMask[64];
extern Bitboard rookAttacksMask[64];
extern Bitboard rookAttacks[64][4096];
extern Bitboard bishopAttacks[64][512];
extern Bitboard line[64][64];
extern Bitboard between[64][64];
void initLineBetween();
void initLeaperAttacks();
void initBishopReleventOccupancyMask();
void initRookReleventOccupancyMask();
void initSliderAttacks();
