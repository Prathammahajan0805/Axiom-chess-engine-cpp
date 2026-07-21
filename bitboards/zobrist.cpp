#include"zobrist.h"
#include "bitboards.h"
uint64_t state = 0x9E3779B97F4A7C15ULL;
Bitboard randomForCastling[16];   // 16 castling-right combinations
Bitboard randomForEnPassant[8];   // One for each file (a-h)
Bitboard randomForSide;           // Black to move
Bitboard randomForZobrist[12][64]; //for everything else
uint64_t random64() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}
void initZobrist(){
    for(int piece=WP;piece<=BK;piece++){
        for(int sq=0;sq<64;sq++){
            randomForZobrist[piece][sq]=random64();
        }
    }
    for (int i = 0; i < 16; i++)
        randomForCastling[i] = random64();
    for (int i = 0; i < 8; i++)
        randomForEnPassant[i] = random64();
    randomForSide = random64();
}