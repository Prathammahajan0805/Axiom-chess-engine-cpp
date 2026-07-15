#pragma once
#include"bitboards.h"
#include"magic_nums.h"
#include <cstdint>

uint64_t magicIndex(Bitboard occupied,Bitboard m,int shift,Bitboard mask);
