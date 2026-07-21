#pragma once
#include <cstdint>
struct Undo{
    int enPassantSq;
    int whiteKingSq;
    int blackKingSq;
    int castlingRights;
    uint64_t hash;
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
