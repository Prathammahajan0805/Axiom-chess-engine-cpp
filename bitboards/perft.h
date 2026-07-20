#pragma once

#include "board.h"
#include"movegen.h"
#include "makeMove.h"

uint64_t perft(Board& board, int depth);
void perftRoot(Board& board, int depth);