#pragma once

#include "board.h"
#include"movegen.h"
#include "makeMove.h"
#include <cstdint>

uint64_t perft(Board& board, int depth);
uint64_t perftRoot(Board& board, int depth);