#pragma once
#include<iostream>
#include<vector>
#include "bitboards.h"
#include "board.hpp"
#include "moves.h"
#include "attacks.h"
#include "types.h"
struct MoveList{
    Move moves[256];
    int count=0;
};
struct checkInfo{
    Bitboard checkers;
    Bitboard checkMask;
    Bitboard pinned;
    Bitboard pinRay[64];
};
void generateCheckersAndPins(const Board &board,checkInfo &info);
template<GenType Type>
void generatePawnMoves(MoveList& createMove,const Board& board,Color color,checkInfo& info);
template<GenType Type>
void generateKnightMoves(MoveList& createMove,checkInfo& info,const Board& board);
template<GenType Type>
void generateKingMoves(MoveList& createMove,checkInfo& info,const Board& board);
template<GenType Type>
void generateSlidingPieces(MoveList& createMove,Piece piece,checkInfo& info,const Board& board);
bool isSquareAttacked(int sq,Color attacker,Bitboard occupied,const Board& board);
template<GenType Type>
MoveList generateMoves(const Board& board);
bool isInCheck(Color color,const Board& board);
bool hasNonePawnPieces(const Board &board);