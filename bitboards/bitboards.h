#pragma once


#include <cassert>
#include <cstdint>
#include <iostream>
#include <bitset>
#include <string>
#include "types.h"


using Bitboard=uint64_t;

enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE
};
enum Rank :int {
    RANKV_1,
    RANKV_2,
    RANKV_3,
    RANKV_4,
    RANKV_5,
    RANKV_6,
    RANKV_7,
    RANKV_8
};
enum File : int{
    FILEV_A,
    FILEV_B,
    FILEV_C,
    FILEV_D,
    FILEV_E,
    FILEV_F,
    FILEV_G,
    FILEV_H
};
inline void setBit(Bitboard &bb,int sq){bb |= (1ULL<<sq); }
inline void clearBit(Bitboard &bb,int sq){bb &= ~(1ULL<<sq); }
inline int getBit(Bitboard bb,int sq){ return (bb>>sq) & 1ULL; }
std::string squareToString(int sq);
inline int popCount(Bitboard bb){
    return __builtin_popcountll(bb);
}

inline int bitScan(Bitboard bb){
    assert(bb);
    return __builtin_ctzll(bb);
}

inline int popLSB(Bitboard& bb){
    assert(bb);
    int sq=bitScan(bb);
    bb &= (bb-1);
    return sq;
}
inline int getSquare(int file,int rank){
    return rank*8+file;
}
constexpr Bitboard FILE_A = 0x0101010101010101ULL;
constexpr Bitboard FILE_B = FILE_A<<1;
constexpr Bitboard FILE_C = FILE_A<<2;
constexpr Bitboard FILE_D = FILE_A<<3;
constexpr Bitboard FILE_E = FILE_A<<4;
constexpr Bitboard FILE_F = FILE_A<<5;
constexpr Bitboard FILE_G = FILE_A<<6;
constexpr Bitboard FILE_H = FILE_A<<7;

constexpr Bitboard RANK_1 = 0xFF;
constexpr Bitboard RANK_2 = RANK_1 << (8 * 1);
constexpr Bitboard RANK_3 = RANK_1 << (8 * 2);
constexpr Bitboard RANK_4 = RANK_1 << (8 * 3);
constexpr Bitboard RANK_5 = RANK_1 << (8 * 4);
constexpr Bitboard RANK_6 = RANK_1 << (8 * 5);
constexpr Bitboard RANK_7 = RANK_1 << (8 * 6);
constexpr Bitboard RANK_8 = RANK_1 << (8 * 7);

constexpr Bitboard NOT_FILE_A = ~(FILE_A);
constexpr Bitboard NOT_FILE_H = ~(FILE_H);
void printBitboard(Bitboard bb);
std::string pieceToUnicode(Piece piece);
char pieceToChar(Piece piece);