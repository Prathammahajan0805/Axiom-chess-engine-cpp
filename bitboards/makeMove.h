#pragma once

#include<vector>
#include "movegen.h"
#include"moves.h"
#include "bitboards.h"
#include "board.h"

struct Undo{
    int enPassantSq;
    int whiteKingSq;
    int blackKingSq;
    int castlingRights;
};

void makeMove(Board& board,MoveList& moveList);
void unMakeMove(Board& board);