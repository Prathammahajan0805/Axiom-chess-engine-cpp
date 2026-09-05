#pragma once
#include <cstdint>
struct Undo{
    int enPassantSq;
    int whiteKingSq;
    int blackKingSq;
    int castlingRights;
    int halfMoveClock;
    uint64_t hash;
    uint64_t pawnHash;
};

enum Piece : int {
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
    NO_PIECE
};
enum dir :int {
    NORTH=8,
    SOUTH=-NORTH,
    EAST=1,
    WEST=-EAST,
    NORTH_EAST=NORTH+EAST,
    NORTH_WEST=NORTH+WEST,
    SOUTH_EAST=SOUTH+EAST,
    SOUTH_WEST=SOUTH+WEST
};
enum Color : int {
    WHITE,
    BLACK,
    BOTH
};

enum Castling : int {
    WHITE_KINGSIDE  = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE  = 4,
    BLACK_QUEENSIDE = 8
};

enum GenType{
    ALL,
    CAPTURES,
    QUITE
};

enum TTFlag{
    EXACT,
    LOWERBOUND,
    UPPERBOUND
};
inline Color pieceColor(Piece piece){return piece>=6?BLACK:WHITE; }