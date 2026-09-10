#pragma once

#include "board.hpp"
#include "bitboards.h"
#include "types.h"

extern Bitboard randomForZobrist[12][64];
extern Bitboard randomForCastling[16];
extern Bitboard randomForEnPassant[8];
extern Bitboard randomForSide;  
void initZobrist();