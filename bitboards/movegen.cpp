#include<iostream>
#include"movegen.h"
#include "bitboards.h"
#include "board.h"

MoveList generateMoves(Board &board){
    MoveList createMove;
    int start=board.sideToMove==WHITE? WP:BP;
    int end=board.sideToMove==WHITE?WK:BK;
    for(int i=start;i<=end;i++){
        Bitboard piece=board.pieces[i];
        while(piece){
            int sq=popLSB(piece);
            switch (i%6) {
                case WP: generatePawnMoves( sq,createMove,board, (Color) board.sideToMove); break;
                case WN:generateKnightMoves( sq, createMove); break;
                case WK:generateKingMoves(sq,createMove);break;
                default:generateSlidingPieces(sq,createMove,(Piece) i);break;
            }
        }
    }
    return createMove;
}