#pragma once

#include<iostream>
#include <string>
#include<vector>
#include"bitboards.h"
#include "moves.h"
#include"types.h"
#include<array>

class Board{
    private:
        void clearBoard();
        std::string const startingFen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        
    public:
        std::array<Piece,64> pieceOn;
        std::array<Undo,512> history;
        std::array<Move,512> moveHistory;
        Bitboard pieces[12];
        Bitboard whitePieces;
        Bitboard blackPieces;
        Bitboard occupied;
        Bitboard hash;
        int ply=0;
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
        std::string toFEN() const;
        void print(Bitboard bb) const;
};