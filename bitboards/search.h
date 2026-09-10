#pragma once

#include "board.hpp"
#include "evaluation.h"
#include "types.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

constexpr int MAX_PLY = 128;
struct SharedState {
    std::atomic<bool> stopSearch{false};
    std::chrono::steady_clock::time_point startTime;
    int64_t timeLimitMs   = 0;
    int64_t optimalTimeMs = 0;
    std::atomic<int64_t> softLimitMs{0};
    std::mutex outputMutex;
    std::atomic<bool> stopFlag{false};
};

struct SearchThread {
    int id = 0;
    Board board;
    PawnHashTable pht; // own instance per thread - eval() takes this by reference, never shared

    // killer moves
    std::array<std::array<Move, 2>, 200> killer{};
    // move history
    std::array<std::array<std::array<int, 64>, 64>, 2> historyHeuristic{};
    // continuation history: [prevPiece][prevTo][movedPiece][to]
    std::array<std::array<std::array<std::array<int, 64>, 6>, 64>, 6> contHistory{};
    // (movedPiece%6, to) of the move just played at each ply
    std::array<std::pair<int, int>, 200> moveStack{};
    // pawn correction history
    std::array<std::array<int, 16384>, 2> pawnCorrHist{};
    // capture history: [movedPiece][to][capturedPiece]
    std::array<std::array<std::array<int, 6>, 64>, 6> captureHistory{};
    // counter moves: [prevMovedPiece][prevTo]
    std::array<std::array<Move, 64>, 6> counterMove{};

    // triangular PV table
    std::array<std::array<Move, MAX_PLY>, MAX_PLY> pvTable{};
    std::array<int, MAX_PLY> pvLength{};
    std::array<int, MAX_PLY> evalHistory{};

    uint64_t nodes = 0;

    // time management (only meaningful / used for thread 0's copy)
    int bestMoveStability = 0;
    Move prevBestMove{0};
    std::array<int, MAX_PLY> scoreHistory{};
    uint64_t rootBestMoveNodes = 0;
    uint64_t rootTotalNodes   = 0;
    int rootBestScore       = -32000;
    int rootSecondBestScore = -32000;

    // result of this thread's iterative deepening, read after join()
    Move bestMove{0};
    int lastScore     = 0;
    int completedDepth = 0;
};

// setup / lifecycle
void initLMR();
void initSearch();                                   // clears TT etc, call once at engine start / ucinewgame
void initSearchThread(SearchThread &ctx, const Board &board);

// entry points used by uci.cpp
void threadLoop(SearchThread &ctx, SharedState &shared, int maxDepth);
SearchThread &pickBestThread(std::vector<SearchThread> &contexts);
uint64_t sumNodes(const std::vector<SearchThread> &contexts);

// misc helpers used elsewhere (uci.cpp move printing etc. stay in uci.cpp)
uint64_t getNodes(const SearchThread &ctx);