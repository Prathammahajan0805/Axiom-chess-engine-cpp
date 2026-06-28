#pragma once

#include <vector>
#include <string>

#include "piece.h"
#include "move.h"
struct Undo {
    int castlingRights;
    int enPassantSquare;
};


class Board {
public:
    Board();

    // Board access
    int getPiece(int rank, int file) const;
    void setPiece(int rank, int file, int piece);

    int getPiece(int index) const;
    void setPiece(int index, int piece);

    // Position
    void loadFEN(const std::string& fen);
    std::string toFEN() const;

    // Debug
    void display() const;

    // Move generation
    std::vector<Move> generateMoves();
    void makeMove(const Move&);
    void unmakeMove();
private:
    
    // Board representation
    std::vector<int> squares;
    int sideToMove;
    int enpassentSquare=-1;
    int castlingRights=15;
    int prevcastlingRights=15;
    // Precomputed attacks
    std::vector<int> knightAttacks[64];
    std::vector<int> kingAttacks[64];
    std::vector<Undo> history;
    std::vector<int> rookRays[64][4];
    std::vector<int> bishopRays[64][4];
    std::vector<Move> moveHistory;

    static const std::string startFEN;

    // Helpers
    bool isopponent(int a, int b);

    // Precomputation
    void precomputeknight();
    void precomputeKing();
    void precomputeRookRays();
    void precomputeBishopRays();

    // Piece move generators
    void generatePawnMoves(int sq, std::vector<Move>& moves);
    void generateKnightMoves(int sq, std::vector<Move>& moves);
    void generateKingMoves(int sq, std::vector<Move>& moves);
    void generateSlidingPieces(int sq, std::vector<Move>& moves);

    // Sliding helper
    void generateRays(const std::vector<int>& ray,int sq,std::vector<Move>& moves);

};