#pragma once
#include"Bitboards.h"
#include"board.h"
#include"types.h"
#include"makeMove.h"
#include"movegen.h"
#include"evaluation.h"

Move findBestMove(Board &board,int depth);
int search(Board& board,int depth,int ply,int alpha,int beta);
uint64_t getNodes();