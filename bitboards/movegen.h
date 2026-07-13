#pragma once
#include<iostream>
#include<vector>
#include "bitboards.h"
#include "board.h"
#include "moves.h"
#include "attacks.h"
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
void generatePawnMoves(int sq,MoveList& createMove,Board& board,Color color);
void generateKnightMoves(int sq,MoveList& createMove);
void generateKingMoves(int sq,MoveList& createMove);
void generateSlidingPieces(int sq,MoveList& createMove,Piece piece);
MoveList generateMoves(Board& board);