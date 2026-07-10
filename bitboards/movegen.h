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
void generatePawnMoves(int sq,MoveList& moves);
void generateKnightMoves(int sq,MoveList& moves);
void generateKingMoves(int sq,MoveList& moves);
void generateSlidingPieces(int sq,MoveList& moves,Piece piece);
void generateMoves(MoveList& moves,Board& board);