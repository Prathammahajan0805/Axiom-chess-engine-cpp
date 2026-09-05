#pragma once
#include"bitboards.h"
#include"board.h"
#include"types.h"
#include"makeMove.h"
#include"movegen.h"
#include"evaluation.h"

Move findBestMove(Board &board,int depth,int64_t timeLimitMs);
int search(Board& board,int depth,int ply,int alpha,int beta,bool isPV, Move excludedMove = {0});
extern PawnHashTable pht;
uint64_t getNodes();
void initSearch();
void initLMR();
void stopSearch();