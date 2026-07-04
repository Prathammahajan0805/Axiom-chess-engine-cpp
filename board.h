#pragma once

#include <vector>
#include <string>

#include "piece.h"
#include "move.h"
struct Undo {
    int castlingRights;
    int enPassantSquare;
};
enum direction{
    VERTICAL=1,
    HORIZONTAL=2,
    RIGHT_DIAG=3,
    LEFT_DIAG=4
};
struct pin{
    int pinned_piece;
    int attacking_piece;
    direction dir;
    std::vector<int> legalSquares;
};
struct checker{
    int attackerSq;
    std::vector<int> blockSquares;
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

    //legal moves
    std::vector<Move> generateLegalMoves(int color);

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
    int whiteKingSquare;
    int blackKingSquare;
    int numCheckers=0;
    // Precomputed attacks
    std::vector<int> knightAttacks[64];
    std::vector<int> kingAttacks[64];
    std::vector<int> rookRays[64][4];
    std::vector<int> bishopRays[64][4];

    //board logics
    std::vector<Undo> history;
    std::vector<Move> moveHistory;
    std::vector<pin> Pins;
    std::vector<checker> Checkers;

    static const std::string startFEN;

    // Helpers
    bool isopponent(int a, int b);

    // Precomputation
    void precomputeknight();
    void precomputeKing();
    void precomputeRookRays();
    void precomputeBishopRays();
    void generateCheckersAndPins(int color);

    // Piece move generators
    void generatePawnMoves(int sq, std::vector<Move>& moves);
    void generateKnightMoves(int sq, std::vector<Move>& moves);
    void generateKingMoves(int sq, std::vector<Move>& moves);
    void generateSlidingPieces(int sq, std::vector<Move>& moves);

    // Sliding helper
    void generateRays(const std::vector<int>& ray,int sq,std::vector<Move>& moves);

    //attack squares
    bool isSquareAttacked(int sq,int opponentColor);

    //check if in check
    bool isInCheck(int MovedColor);
    bool isSquareBlockingCheck(int sq);
    bool isCheckersSquare(int sq);
    bool isPiecePinned(int sq);
    std::vector<Move> legalityFilter(std::vector<Move>& legalmoves);
    bool checkPinLimits(pin p,int sq);
    pin givePinInfo(int sq);
};