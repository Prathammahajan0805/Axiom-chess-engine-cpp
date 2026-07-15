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
void generateCheckersAndPins(Board &board,checkInfo &info);
void generatePawnMoves(MoveList& createMove,Board& board,Color color,checkInfo& info);
void generateKnightMoves(int sq,MoveList& createMove,checkInfo& info);
void generateKingMoves(int sq,MoveList& createMove,checkInfo& info);
void generateSlidingPieces(int sq,MoveList& createMove,Piece piece,checkInfo& info);
MoveList generateMoves(Board& board);