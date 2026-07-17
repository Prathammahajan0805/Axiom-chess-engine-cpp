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
void generateKnightMoves(MoveList& createMove,checkInfo& info,Board& board);
void generateKingMoves(MoveList& createMove,checkInfo& info,Board& board);
void generateSlidingPieces(MoveList& createMove,Piece piece,checkInfo& info,Board& board);
bool isSquareAttacked(int sq,Color attacker,Bitboard occupied,const Board& board);
MoveList generateMoves(Board& board);