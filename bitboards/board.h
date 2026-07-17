#pragma once

#include<iostream>
#include<vector>
#include"bitboards.h"
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

class Board{
    private:
        void clearBoard();
        std::string const startingFen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        
    public:
        std::vector<Piece> pieceOn;
        Bitboard pieces[12];
        Bitboard whitePieces;
        Bitboard blackPieces;
        Bitboard occupied;

        int sideToMove;
        int castlingRights;
        int enPassantSquare;
        int whiteKingSquare;
        int blackKingSquare;


        Board();

        inline void updateOccupancies(){
            whitePieces =
                pieces[WP] | pieces[WN] | pieces[WB] |
                pieces[WR] | pieces[WQ] | pieces[WK];

            blackPieces =
                pieces[BP] | pieces[BN] | pieces[BB] |
                pieces[BR] | pieces[BQ] | pieces[BK];

            occupied = whitePieces | blackPieces;
        }
        void loadFEN(const std::string &fen);

        void print() const;
};