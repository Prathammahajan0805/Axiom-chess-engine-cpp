#include "search.h"
#include "TT.h"
#include "board.hpp"
#include "evaluation.h"
#include "makeMove.h"
#include "movegen.h"
#include "moves.h"
#include "see.h"
#include "types.h"
#include "uci.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <chrono>
#include <cmath>
#include <iostream>

constexpr int INF = 32000;
constexpr int PROMOTION_PRIOR = 10000;
constexpr int TTMOVE_SCORE = 100000;
constexpr int MAX_HISTORY = 16000;
constexpr int MIN_HISTORY = -MAX_HISTORY;
constexpr int CORR_HISTORY_LIMIT = 16384;
constexpr int CORR_SCALE = 256;

// Read-only after initLMR() runs once at startup -> safe to share across threads.
std::array<std::array<int, 64>, 64> reductionTable;

bool isInsufficientMaterial(const Board &board) {
    if (board.pieces[WP] | board.pieces[BP] |
        board.pieces[WN] | board.pieces[BN] |
        board.pieces[WR] | board.pieces[BR] |
        board.pieces[WQ] | board.pieces[BQ]) {
        return false;
    }

    int whiteKnights = popCount(board.pieces[WN]);
    int blackKnights = popCount(board.pieces[BN]);
    int whiteBishops = popCount(board.pieces[WB]);
    int blackBishops = popCount(board.pieces[BB]);

    int totalMinors = whiteKnights + blackKnights + whiteBishops + blackBishops;

    if (totalMinors == 0) return true; // K vs K
    if (totalMinors == 1) return true; // K+minor vs K

    if (totalMinors == 2 && whiteBishops == 1 && blackBishops == 1) {
        int wb = bitScan(board.pieces[WB]);
        int bb = bitScan(board.pieces[BB]);
        return (wb % 2) == (bb % 2);
    }

    return false;
}

bool isRepetition(const Board &board) {
    for (int i = board.ply - 2; i >= std::max(0, board.ply - board.halfMoveClock); i -= 2) {
        if (board.hash == board.hashHistory[i]) return true;
    }
    return false;
}

int scoreToTT(int score, int ply) {
    if (score >= MATE_SCORE - 1000) return score + ply;
    if (score <= -MATE_SCORE + 1000) return score - ply;
    return score;
}

int scoreFromTT(int score, int ply) {
    if (score >= MATE_SCORE - 1000) return score - ply;
    if (score <= -MATE_SCORE + 1000) return score + ply;
    return score;
}

int captureValue(const Move &move) {
    return SEE_VALUE[getCapturedPiece(move) % 6];
}

void initLMR() {
    for (int d = 1; d < 64; d++)
        for (int m = 1; m < 64; m++)
            reductionTable[d][m] = (int)(0.75 + log(d) * log(m) / 2.25); // to tune
}

void initSearch() {
    clearTT();
}

void clearKillers(SearchThread &ctx) {
    Move empty{0};
    std::array<Move, 2> emptyKillers{empty, empty};
    ctx.killer.fill(emptyKillers);
}

void initSearchThread(SearchThread &ctx, const Board &board) {
    ctx.board = board; // deep copy Board is a value type, see board.h
    ctx.pht = PawnHashTable{}; // fresh per-thread pawn hash table for this search

    ctx.historyHeuristic = {};
    ctx.contHistory = {};
    ctx.captureHistory = {};
    ctx.pawnCorrHist = {};
    ctx.counterMove = {};
    clearKillers(ctx);
    ctx.moveStack = {};

    ctx.evalHistory.fill(0);
    ctx.pvLength.fill(0);
    ctx.pvTable = {};

    ctx.nodes = 0;

    ctx.bestMoveStability = 0;
    ctx.prevBestMove = Move{0};
    ctx.scoreHistory.fill(0);
    ctx.rootBestMoveNodes = 0;
    ctx.rootTotalNodes = 0;
    ctx.rootBestScore = -INF;
    ctx.rootSecondBestScore = -INF;

    ctx.bestMove = Move{0};
    ctx.lastScore = 0;
    ctx.completedDepth = 0;
}

uint64_t getNodes(const SearchThread &ctx) { return ctx.nodes; }

uint64_t sumNodes(const std::vector<SearchThread> &contexts) {
    uint64_t total = 0;
    for (const auto &c : contexts) total += c.nodes;
    return total;
}

int correctedStaticEval(SearchThread &ctx, const Board &board, int rawEval) {
    int corr = ctx.pawnCorrHist[board.sideToMove][board.pawnHash & (CORR_HISTORY_LIMIT - 1)];
    int adjusted = rawEval + corr / CORR_SCALE;
    return std::clamp(adjusted, -MATE_SCORE + MAX_PLY, MATE_SCORE - MAX_PLY);
}

void updateCorrHist(SearchThread &ctx, const Board &board, int depth, int bestScore, int staticEval) {
    int &entry = ctx.pawnCorrHist[board.sideToMove][board.pawnHash & (CORR_HISTORY_LIMIT - 1)];
    int diff = (bestScore - staticEval) * CORR_SCALE;
    int weight = std::min(depth, 16);
    entry += (diff - entry) * weight / 32;
    entry = std::clamp(entry, -CORR_SCALE * 128, CORR_SCALE * 128);
}

int64_t computeSoftLimit(SearchThread &ctx, SharedState &shared, int depth) {
    if (depth <= 5) return shared.optimalTimeMs;

    float stabilityTable[] = {1.50, 1.2, 1, 0.9, 0.8, 0.75};
    int64_t idx = std::min(ctx.bestMoveStability, 5);
    float stabilityFactor = stabilityTable[idx];

    int64_t delta = ctx.scoreHistory[depth] - ctx.scoreHistory[depth - 1];
    double scoreFactor;
    if (delta <= -40) scoreFactor = 1.5;
    else if (delta <= -10) scoreFactor = 1.2;
    else if (delta >= 40) scoreFactor = 1.1;
    else scoreFactor = 1.0;

    double nodesFactor = 1.0;
    if (ctx.rootTotalNodes > 0) {
        double frac = (double)ctx.rootBestMoveNodes / (double)ctx.rootTotalNodes;
        nodesFactor = std::clamp((1.0 - frac) * 2.0, 0.5, 1.5);
    }

    double newSoft = shared.optimalTimeMs * stabilityFactor * scoreFactor * nodesFactor;
    return (int64_t)std::clamp(newSoft, shared.optimalTimeMs * 0.4, (double)shared.timeLimitMs);
}

int getCaptureScore(SearchThread &ctx, const Move &move, const Board &board) {
    int score = 0;
    Piece moved = (Piece)(getMovedPiece(move) % 6);
    Piece captured = (Piece)(getCapturedPiece(move) % 6);
    score += (captured - moved + 5) * 1000;
    score += ctx.captureHistory[moved][getTo(move)][captured] / 4;
    if (see(board, move) < 0) score -= 30000;
    else score += 20000;
    return score;
}

int scoreMove(SearchThread &ctx, const Move &move, int ply, const Board &board) {
    bool isPromotion = getPromotionPiece(move) != NO_PIECE;
    bool iscapture = isCapture(move);
    int score = 0;
    if (isPromotion) score += PROMOTION_PRIOR;
    if (iscapture) score += getCaptureScore(ctx, move, board);

    if (ctx.killer[ply][0].moveInfo == move.moveInfo) score += 6000;
    if (ctx.killer[ply][1].moveInfo == move.moveInfo) score += 5000;

    if (ply > 0) {
        int prevPiece = ctx.moveStack[ply - 1].first, prevTo = ctx.moveStack[ply - 1].second;
        score += ctx.contHistory[prevPiece][prevTo][getMovedPiece(move) % 6][getTo(move)];
        if (ctx.counterMove[prevPiece][prevTo].moveInfo == move.moveInfo) score += 4000;
    }

    score += ctx.historyHeuristic[int(pieceColor(Piece(getMovedPiece(move))))][getFrom(move)][getTo(move)];
    return score;
}

bool checkTime(SearchThread &ctx, SharedState &shared) {
    if ((ctx.nodes & 2047) != 0) return shared.stopSearch.load(std::memory_order_relaxed);

    if (ctx.id == 0 && !shared.isPondering.load(std::memory_order_relaxed)) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - shared.startTime).count();
        if (elapsed >= shared.timeLimitMs) {
            shared.stopSearch.store(true, std::memory_order_relaxed);
        }
    }
    return shared.stopSearch.load(std::memory_order_relaxed);
}

int quiescence(SearchThread &ctx, SharedState &shared, Board &board, int alpha, int beta, int ply) {
    ctx.nodes++;
    if (ply >= MAX_PLY - 1) return eval(board, ctx.pht);
    if (checkTime(ctx, shared)) return 0;

    bool inCheck = isInCheck((Color)board.sideToMove, board);
    MoveList moveList = inCheck ? generateMoves<ALL>(board) : generateMoves<CAPTURES>(board);
    int standPat = eval(board, ctx.pht);

    if (!inCheck) {
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;
    }

    if (moveList.count == 0) {
        if (inCheck) return -MATE_SCORE + ply;
        return alpha;
    }

    std::array<int, 256> scores;
    for (int i = 0; i < moveList.count; i++) {
        if (!inCheck) {
            scores[i] = getCaptureScore(ctx, moveList.moves[i], board);
        } else {
            scores[i] = ctx.historyHeuristic[pieceColor(Piece(getMovedPiece(moveList.moves[i])))]
                                             [getFrom(moveList.moves[i])][getTo(moveList.moves[i])];
        }
    }

    for (int i = 0; i < moveList.count; i++) {
        int maxScoreMove = scores[i], ind = i;
        for (int j = i; j < moveList.count; j++) {
            if (maxScoreMove < scores[j]) {
                maxScoreMove = scores[j];
                ind = j;
            }
        }
        std::swap(moveList.moves[i], moveList.moves[ind]);
        std::swap(scores[i], scores[ind]);

        if (!inCheck && standPat + captureValue(moveList.moves[i]) + 200 < alpha) continue;
        if (!inCheck && isCapture(moveList.moves[i]) && !seeGE(board, moveList.moves[i], 0)) continue;

        makeMove(board, moveList.moves[i]);
        int score = -quiescence(ctx, shared, board, -beta, -alpha, ply + 1);
        unMakeMove(board);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int search(SearchThread &ctx, SharedState &shared, Board &board, int depth, int ply,
           int alpha, int beta, bool isPV, Move excludedMove = Move{0}) {
    ctx.nodes++;
    if (ply < MAX_PLY) ctx.pvLength[ply] = ply;
    if (ply >= MAX_PLY - 1) return eval(board, ctx.pht);
    if (checkTime(ctx, shared) ||
        (ply > 0 && (isRepetition(board) || board.halfMoveClock >= 100)) ||
        isInsufficientMaterial(board))
        return 0;

    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta = std::min(beta, MATE_SCORE - ply - 1);
    if (alpha >= beta) return alpha;

    bool isCheck = isInCheck((Color)board.sideToMove, board);

    int rawEval = eval(board, ctx.pht);
    int staticEval = correctedStaticEval(ctx, board, rawEval);

    // probCut
    if (depth >= 5 && !isPV && !isCheck && std::abs(beta) < MATE_SCORE - 1000) {
        int pcBeta = beta + 150; // tune

        if (staticEval + SEE_VALUE[WQ] >= pcBeta) {
            MoveList pcMoves = generateMoves<CAPTURES>(board);
            std::array<int, 256> pcScores;
            for (int i = 0; i < pcMoves.count; i++)
                pcScores[i] = getCaptureScore(ctx, pcMoves.moves[i], board);

            for (int i = 0; i < pcMoves.count; i++) {
                int maxScore = pcScores[i], ind = i;
                for (int j = i; j < pcMoves.count; j++)
                    if (pcScores[j] > maxScore) { maxScore = pcScores[j]; ind = j; }
                std::swap(pcMoves.moves[i], pcMoves.moves[ind]);
                std::swap(pcScores[i], pcScores[ind]);

                Move &m = pcMoves.moves[i];
                if (!seeGE(board, m, pcBeta - staticEval)) continue;

                makeMove(board, m);
                int score = -search(ctx, shared, board, depth - 4, ply + 1, -pcBeta, -pcBeta + 1, false);
                unMakeMove(board);
                if (shared.stopSearch.load(std::memory_order_relaxed)) return 0;

                if (score >= pcBeta) return score;
            }
        }
    }

    if (depth == 0) return quiescence(ctx, shared, board, alpha, beta, ply);

    Move bestMove{0};
    TTEntry *entry = probe(board.hash);
    bool singularCandidate = entry && !excludedMove.moveInfo && ply > 0 && depth >= 8 &&
                              entry->depth >= depth - 3 && entry->flag != UPPERBOUND &&
                              std::abs(entry->score) < MATE_SCORE - 1000;

    int extension = 0;
    Move ttMove = entry ? entry->bestMove : Move{0};

    if (singularCandidate) {
        int sBeta = scoreFromTT(entry->score, ply) - 2 * depth;
        int sDepth = (depth - 1) / 2;
        int sScore = search(ctx, shared, board, sDepth, ply, sBeta - 1, sBeta, false, ttMove);
        if (sScore < sBeta) {
            extension = 1;
        } else if (sBeta >= beta) {
            return sBeta;
        }
    }

    if (entry) {
        ttMove = entry->bestMove;
        if (entry->depth >= depth && ply != 0) {
            if (entry->flag == EXACT) return scoreFromTT(entry->score, ply);
            if (entry->flag == LOWERBOUND) alpha = std::max(alpha, scoreFromTT(entry->score, ply));
            if (entry->flag == UPPERBOUND) beta = std::min(beta, scoreFromTT(entry->score, ply));
            if (alpha >= beta) return scoreFromTT(entry->score, ply);
        }
    }

    if (!entry && depth >= 4 && !isCheck) {
        depth -= 1;
    }

    ctx.evalHistory[ply] = staticEval;
    bool improving = (ply >= 2 && staticEval > ctx.evalHistory[ply - 2]);

    // reverse futility pruning
    if (!isCheck && depth <= 6 && ply != 0 && beta < MATE_SCORE - 1000) {
        int baseMargin = 120 * depth;
        int margin = baseMargin + (improving ? 0 : 30);
        if (staticEval - margin >= beta) return staticEval - margin;
    }

    // razoring
    if (!isPV && !isCheck && depth <= 3 && staticEval + depth * 120 < alpha) {
        int qScore = quiescence(ctx, shared, board, alpha, beta, ply);
        if (qScore < alpha) return qScore;
    }

    MoveList moveList = generateMoves<ALL>(board);
    int bestScore = -INF;

    if (moveList.count == 0) {
        if (isCheck) return -MATE_SCORE + ply;
        return DRAW_SCORE;
    }

    std::array<int, 256> scores;
    for (int i = 0; i < moveList.count; i++) {
        if (moveList.moves[i].moveInfo == ttMove.moveInfo) scores[i] = TTMOVE_SCORE;
        else scores[i] = scoreMove(ctx, moveList.moves[i], ply, board);
    }

    // null move pruning
    if (depth >= 3 && ply != 0 && !isCheck && hasNonePawnPieces(board) && beta < MATE_SCORE - 1000) {
        makeNullMove(board);
        int R = 2 + depth / 6;
        int score = -search(ctx, shared, board, depth - 1 - R, ply + 1, -beta, -beta + 1, false);
        unmakeNullMove(board);
        if (shared.stopSearch.load(std::memory_order_relaxed)) return 0;
        if (score >= beta) return beta;
    }

    int newDepth = depth - 1 + (isCheck ? 1 : 0);
    int origAlpha = alpha;
    int quietsTried = 0;

    for (int i = 0; i < moveList.count; i++) {
        int maxScoreMove = scores[i], ind = i;
        for (int j = i; j < moveList.count; j++) {
            if (maxScoreMove < scores[j]) { maxScoreMove = scores[j]; ind = j; }
        }
        std::swap(moveList.moves[i], moveList.moves[ind]);
        std::swap(scores[i], scores[ind]);

        Move &move = moveList.moves[i];
        bool isQuiet = getCapturedPiece(move) == NO_PIECE && getPromotionPiece(move) == NO_PIECE;

        if (excludedMove.moveInfo && move.moveInfo == excludedMove.moveInfo) continue;
        int thisNewDepth = newDepth + (move.moveInfo == ttMove.moveInfo ? extension : 0);

        bool isTTMove = ttMove.moveInfo && move.moveInfo == ttMove.moveInfo;
        bool isKillerMove = ctx.killer[ply][0].moveInfo == move.moveInfo ||
                             ctx.killer[ply][1].moveInfo == move.moveInfo;
        bool isCounterMove = false;
        int quietHistory = 0;
        if (ply > 0) {
            auto [prevPiece, prevTo] = ctx.moveStack[ply - 1];
            isCounterMove = ctx.counterMove[prevPiece][prevTo].moveInfo == move.moveInfo;
            quietHistory += ctx.contHistory[prevPiece][prevTo][getMovedPiece(move) % 6][getTo(move)];
        }
        int moverColor = int(pieceColor(Piece(getMovedPiece(move))));
        quietHistory += ctx.historyHeuristic[moverColor][getFrom(move)][getTo(move)];

        bool pruningExempt = isTTMove || isKillerMove || isCounterMove || quietHistory > 4000;

        if (!isPV && !isCheck && !pruningExempt && depth <= 8 && bestScore > -MATE_SCORE + 1000 && isQuiet) {
            int lmpThreshold = 3 + 2 * depth * depth + (improving ? depth : 0);
            if (quietsTried >= lmpThreshold) continue;
        }
        if (isQuiet) quietsTried++;

        if (!isPV && !isCheck && !pruningExempt && depth <= 8 && isQuiet && bestScore > -MATE_SCORE + 1000) {
            int futilityMargin = 100 + 90 * depth;
            futilityMargin += (improving ? 0 : 30);
            futilityMargin += std::clamp(quietHistory / 64, 0, 200);
            if (staticEval + futilityMargin <= alpha) continue;
        }

        if (!isPV && !isCheck && !pruningExempt && depth <= 8 && bestScore > -MATE_SCORE + 1000) {
            int seeMargin = isCapture(move) ? -20 * depth * depth : -50 * depth;
            seeMargin -= improving ? 0 : 30;
            if (!seeGE(board, move, seeMargin)) continue;
        }

        ctx.moveStack[ply] = {getMovedPiece(move) % 6, getTo(move)};
        uint64_t nodesBeforeThisMove = ctx.nodes;

        makeMove(board, move);
        int score;
        if (i == 0) {
            score = -search(ctx, shared, board, thisNewDepth, ply + 1, -beta, -alpha, isPV);
        } else {
            bool neededScan = true;
            bool givesCheck = isInCheck((Color)board.sideToMove, board);

            if (depth >= 3 && i >= 3 && getCapturedPiece(move) == NO_PIECE && !isCheck && !givesCheck) {
                int red = reductionTable[std::min(depth, 63)][std::min(i, 63)];
                red -= quietHistory / 8192;
                if (!improving) red += 1;
                red = std::clamp(red, 0, newDepth);
                int reducedDepth = std::max(thisNewDepth - red, 0);

                score = -search(ctx, shared, board, reducedDepth, ply + 1, -alpha - 1, -alpha, false);
                neededScan = score > alpha;
            }
            if (neededScan) {
                score = -search(ctx, shared, board, thisNewDepth, ply + 1, -alpha - 1, -alpha, false);
                if (score > alpha && score < beta) {
                    score = -search(ctx, shared, board, thisNewDepth, ply + 1, -beta, -alpha, isPV);
                }
            }
        }
        unMakeMove(board);

        if (ply == 0 && score > bestScore) {
            ctx.rootBestMoveNodes = ctx.nodes - nodesBeforeThisMove;
        }
        if (ply == 0) {
            if (score > ctx.rootBestScore) {
                ctx.rootSecondBestScore = ctx.rootBestScore;
                ctx.rootBestScore = score;
            } else if (score > ctx.rootSecondBestScore) {
                ctx.rootSecondBestScore = score;
            }
        }

        if (shared.stopSearch.load(std::memory_order_relaxed)) return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha) {
            alpha = score;
            ctx.pvTable[ply][ply] = move;
            for (int nextPly = ply + 1; nextPly < ctx.pvLength[ply + 1]; nextPly++) {
                ctx.pvTable[ply][nextPly] = ctx.pvTable[ply + 1][nextPly];
            }
            ctx.pvLength[ply] = ctx.pvLength[ply + 1];
        }

        if (alpha >= beta) {
            if (getPromotionPiece(move) == NO_PIECE && !isCapture(move)) {
                int bonus = depth * depth;
                int from = getFrom(move), to = getTo(move);
                auto &h = ctx.historyHeuristic[board.sideToMove][from][to];
                h += bonus - (h * std::abs(bonus) / MAX_HISTORY);
                if (ply > 0) {
                    auto [prevPiece, prevTo] = ctx.moveStack[ply - 1];
                    auto &ch = ctx.contHistory[prevPiece][prevTo][getMovedPiece(move) % 6][to];
                    ch += bonus - (ch * std::abs(bonus) / MAX_HISTORY);
                }
            }
            if (isCapture(move)) {
                Piece moved = (Piece)(getMovedPiece(move) % 6), captured = (Piece)(getCapturedPiece(move) % 6);
                int bonus = depth * depth;
                auto &ch = ctx.captureHistory[moved][getTo(move)][captured];
                ch += bonus - (ch * std::abs(bonus) / MAX_HISTORY);
            }
            if (getPromotionPiece(move) == NO_PIECE && !isCapture(move) && ctx.killer[ply][0].moveInfo != move.moveInfo) {
                if (ctx.killer[ply][0].moveInfo == 0) {
                    ctx.killer[ply][0] = move;
                } else {
                    ctx.killer[ply][1] = ctx.killer[ply][0];
                    ctx.killer[ply][0] = move;
                }
            }
            if (ply > 0 && getPromotionPiece(move) == NO_PIECE && !isCapture(move)) {
                auto [prevPiece, prevTo] = ctx.moveStack[ply - 1];
                ctx.counterMove[prevPiece][prevTo] = move;
            }
            for (int j = 0; j < i; j++) {
                Move &prev = moveList.moves[j];
                if (getPromotionPiece(prev) == NO_PIECE && !isCapture(prev)) {
                    int malus = -depth * depth;
                    int from = getFrom(prev), to = getTo(prev);
                    auto &h = ctx.historyHeuristic[board.sideToMove][from][to];
                    h += malus - (h * std::abs(malus) / MAX_HISTORY);
                    if (ply > 0) {
                        auto [prevPiece, prevTo] = ctx.moveStack[ply - 1];
                        auto &ch = ctx.contHistory[prevPiece][prevTo][getMovedPiece(prev) % 6][to];
                        ch += malus - (ch * std::abs(malus) / MAX_HISTORY);
                    }
                }
            }
            break;
        }
    }

    if (shared.stopSearch.load(std::memory_order_relaxed)) return 0;

    TTFlag flag;
    if (bestScore <= origAlpha) flag = UPPERBOUND;
    else if (bestScore >= beta) flag = LOWERBOUND;
    else flag = EXACT;

    if (!isCheck && getPromotionPiece(bestMove) == NO_PIECE)
        updateCorrHist(ctx, board, depth, bestScore, staticEval);

    if (!excludedMove.moveInfo) {
        store(board.hash, bestMove, depth, scoreToTT(bestScore, ply), flag);
    }

    return bestScore;
}

void threadLoop(SearchThread &ctx, SharedState &shared, int maxDepth) {
    MoveList rootMoves = generateMoves<ALL>(ctx.board);
    if (rootMoves.count == 0) return;

    ctx.bestMove = rootMoves.moves[0];
    int score = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {
        uint64_t nodesAtIterationStart = ctx.nodes;

        int alphaWindow = 25, betaWindow = 25;
        int alpha = score - alphaWindow;
        int beta = score + betaWindow;

        while (true) {
            ctx.rootBestScore = -INF;
            ctx.rootSecondBestScore = -INF;
            int newScore = search(ctx, shared, ctx.board, depth, 0, alpha, beta, true);

            if (shared.stopSearch.load(std::memory_order_relaxed)) break;

            if (newScore <= alpha) {
                alphaWindow *= 2;
                alpha = score - alphaWindow;
            } else if (newScore >= beta) {
                betaWindow *= 2;
                beta = score + betaWindow;
            } else {
                score = newScore;
                break;
            }
        }

        if (shared.stopSearch.load(std::memory_order_relaxed)) break;

        ctx.bestMove = ctx.pvTable[0][0];
        ctx.lastScore = score;
        ctx.completedDepth = depth;

        if (ctx.bestMove.moveInfo == ctx.prevBestMove.moveInfo) {
            ctx.bestMoveStability = std::min(ctx.bestMoveStability + 1, 10);
        } else {
            ctx.bestMoveStability = 0;
        }
        ctx.prevBestMove = ctx.bestMove;
        ctx.scoreHistory[depth] = ctx.lastScore;
        ctx.rootTotalNodes = ctx.nodes - nodesAtIterationStart;

        if (ctx.id == 0) {
            shared.softLimitMs.store(computeSoftLimit(ctx, shared, depth), std::memory_order_relaxed);

            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - shared.startTime).count();
            uint64_t nps = elapsedMs > 0 ? (ctx.nodes * 1000) / elapsedMs : 0;

            std::string scoreStr;
            if (ctx.lastScore >= MATE_SCORE - 1000) {
                int mateIn = (MATE_SCORE - ctx.lastScore + 1) / 2;
                scoreStr = "mate " + std::to_string(mateIn);
            } else if (ctx.lastScore <= -MATE_SCORE + 1000) {
                int mateIn = (-MATE_SCORE - ctx.lastScore) / 2;
                scoreStr = "mate " + std::to_string(mateIn);
            } else {
                scoreStr = "cp " + std::to_string(ctx.lastScore);
            }

            {
                std::lock_guard<std::mutex> lock(shared.outputMutex);
                std::cout << "info depth " << depth
                          << " score " << scoreStr
                          << " nodes " << ctx.nodes
                          << " nps " << nps
                          << " time " << elapsedMs
                          << " pv";
                for (int i = 0; i < ctx.pvLength[0]; i++)
                    std::cout << " " << moveToUci(ctx.pvTable[0][i]);
                std::cout << std::endl;
            }

            bool easyMove = depth >= 6 && ctx.bestMoveStability >= 4 &&
                             (ctx.rootBestScore - ctx.rootSecondBestScore) > 50;

            if (elapsedMs >= shared.softLimitMs.load(std::memory_order_relaxed)) {
                shared.stopSearch.store(true, std::memory_order_relaxed);
            }
            if (easyMove && elapsedMs >= shared.optimalTimeMs * 0.5) {
                shared.stopSearch.store(true, std::memory_order_relaxed);
            }
        }

        if (shared.stopSearch.load(std::memory_order_relaxed)) break;
    }
}


SearchThread &pickBestThread(std::vector<SearchThread> &contexts) {
    SearchThread *best = &contexts[0];
    for (auto &c : contexts) {
        if (c.completedDepth > best->completedDepth) {
            best = &c;
        } else if (c.completedDepth == best->completedDepth && c.lastScore > best->lastScore) {
            best = &c;
        }
    }
    return *best;
}