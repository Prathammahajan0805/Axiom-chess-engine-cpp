#include "makeMove.h"
#include "bitboards.h"
#include "board.h"
#include "moves.h"
#include "types.h"
#include "zobrist.h"
#include <cassert>
using namespace std;



void makeMove(Board &board,Move &move){
    //fix memory overflow caused by long nmatches
    assert(board.ply < 511 && "ply overflow: history array full");
    board.moveHistory[board.ply]=move;
    board.hashHistory[board.ply]=board.hash;
    board.history[board.ply++]={board.enPassantSquare,board.whiteKingSquare,
    board.blackKingSquare,board.castlingRights,board.halfMoveClock,board.hash,board.pawnHash};
    Color color=(Color) board.sideToMove;
    int myColOff=color*6;
    if(board.enPassantSquare!=NO_SQUARE) board.hash ^= randomForEnPassant[board.enPassantSquare%8];
    board.enPassantSquare=NO_SQUARE;
    int oldCastling=board.castlingRights;

    //common
    //get from to
    int from=getFrom(move);
    int to=getTo(move);
    Piece piece=(Piece) getMovedPiece(move);
    //update mailbox
    board.pieceOn[from]=NO_PIECE;
    board.pieceOn[to]=(Piece) piece;
    bool capture=isCapture(move);

    //update halfmove
    board.halfMoveClock++;
    if(piece==WP || piece==BP || capture) board.halfMoveClock=0;

    //create a from to mask
    Bitboard fromToMask=(1ULL<<from);
    fromToMask |=(1ULL<<to);
    board.pieces[piece] ^= fromToMask;

    //update zobrist hash
    board.hash ^= randomForZobrist[piece][from]; //remove from
    board.hash ^= randomForZobrist[piece][to]; //add to

    //pawn hash for eval
    if(piece==WP || piece==BP){
        board.pawnHash ^=randomForZobrist[piece][to];//add to
        board.pawnHash ^=randomForZobrist[piece][from];//remove from
    }

    if(color==WHITE){
        board.whitePieces ^= fromToMask;
    }
    else{
        board.blackPieces ^= fromToMask;
    }
    board.occupied ^= fromToMask;

    //capture pieces
    if(capture && !isEnPassant(move)){
        board.pieces[getCapturedPiece(move)] ^= (1ULL<<to);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<<to);
        }
        else{
            board.blackPieces ^= (1ULL<<to);
        }
        board.occupied ^= (1ULL<<to);
        board.hash ^= randomForZobrist[getCapturedPiece(move)][to];
        switch(to) {
            case A1:
                board.castlingRights &= ~WHITE_QUEENSIDE;
                break;
            case H1:
                board.castlingRights &= ~WHITE_KINGSIDE;
                break;
            case A8:
                board.castlingRights &= ~BLACK_QUEENSIDE;
                break;
            case H8:
                board.castlingRights &= ~BLACK_KINGSIDE;
                break;
        }
        
    }

    //double pawn push
    if(isDoublePawnPush(move)){
        if(piece==WP){
            board.enPassantSquare=to-8;
        }
        else board.enPassantSquare=to+8;
    }

    //promotion
    Piece promotionP=(Piece) getPromotionPiece(move);
    if(promotionP!=NO_PIECE){
        setBit(board.pieces[promotionP],to);
        board.pieces[piece] ^= (1ULL<<to);
        board.pieceOn[to]=promotionP;

        board.hash ^= randomForZobrist[piece][to];
        board.hash ^= randomForZobrist[promotionP][to];

        board.pawnHash^=randomForZobrist[piece][to];
    }
    
    //enpassant
    if(isEnPassant(move)){
        int pawnOn=(color==WHITE?to-8:to+8);
        board.pieces[getCapturedPiece(move)] ^= (1ULL<< pawnOn);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<< pawnOn);
        }else{
            board.blackPieces ^= (1ULL<< pawnOn);
        }
        board.occupied ^= (1ULL<<pawnOn);
        board.pieceOn[pawnOn]=NO_PIECE;
        board.hash ^= randomForZobrist[getCapturedPiece(move)][pawnOn];
        board.pawnHash^= randomForZobrist[getCapturedPiece(move)][pawnOn];
    }

    //castling
    if(isCastle(move)){
        Bitboard rookFromTo=0ULL;
        if(to==from+2){
            rookFromTo= (1ULL<< (to+1));
            rookFromTo |= (1ULL << (to -1));
            board.pieces[WR+myColOff] ^=rookFromTo;
            board.pieceOn[to+1]=NO_PIECE;
            board.pieceOn[to-1]=(Piece) (WR+myColOff);

            board.hash ^= randomForZobrist[WR + myColOff][to + 1];
            board.hash ^= randomForZobrist[WR + myColOff][to - 1];
        }else if(to==from-2){
            rookFromTo= (1ULL<< (to+1));
            rookFromTo |= (1ULL << (to-2));
            board.pieces[WR+myColOff] ^=rookFromTo;
            board.pieceOn[to-2]=NO_PIECE;
            board.pieceOn[to+1]=(Piece) (WR+myColOff);

            board.hash ^= randomForZobrist[WR + myColOff][to - 2];
            board.hash ^= randomForZobrist[WR + myColOff][to + 1];
        }
        if(color==WHITE){
            board.whitePieces ^=rookFromTo;
        }else{
            board.blackPieces ^=rookFromTo;
        }
        board.occupied ^=rookFromTo;
    }
    //remove castling rights
    if(piece==WK+myColOff){
        if(color==WHITE){
            board.whiteKingSquare=to;
            board.castlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
        }else{
            board.blackKingSquare=to;
            board.castlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
        }   
    }

    //remove castling rights for rook
    if(piece==WR+myColOff){
        switch (from) {
            case A1: board.castlingRights &= ~WHITE_QUEENSIDE; break;
            case H1: board.castlingRights &= ~WHITE_KINGSIDE; break;
            case A8: board.castlingRights &= ~BLACK_QUEENSIDE; break;
            case H8: board.castlingRights &= ~BLACK_KINGSIDE; break;
        }
    }
    
    board.sideToMove ^= 1;
    if(oldCastling!=board.castlingRights){
        board.hash ^=randomForCastling[oldCastling];
        board.hash ^=randomForCastling[board.castlingRights];
    }
    if(board.enPassantSquare!=NO_SQUARE) board.hash ^= randomForEnPassant[board.enPassantSquare%8];
    board.hash ^= randomForSide;
}

void unMakeMove(Board &board){
    assert(board.ply > 0 && board.ply <= 512 && "unMake: ply underflow");
    Move move=board.moveHistory[--board.ply];
    Undo undo=board.history[board.ply];

    //undo
    board.enPassantSquare=undo.enPassantSq;
    board.castlingRights=undo.castlingRights;
    board.blackKingSquare=undo.blackKingSq;
    board.whiteKingSquare=undo.whiteKingSq;
    board.halfMoveClock=undo.halfMoveClock;
    board.hash=undo.hash;
    board.pawnHash=undo.pawnHash;

    Color color=(Color)( board.sideToMove ^ 1);
    int myColOff=color*6;

    //common
    int from=getFrom(move);
    int to=getTo(move);
    board.pieceOn[to]=NO_PIECE;
    board.pieceOn[from]=(Piece) getMovedPiece(move);
    Bitboard fromToMask=(1ULL<<from);
    fromToMask |=(1ULL<<to);
    board.pieces[getMovedPiece(move)] ^= fromToMask;
    if(color==WHITE){
        board.whitePieces ^= fromToMask;
    }
    else{
        board.blackPieces ^= fromToMask;
    }
    board.occupied ^= fromToMask;

    //capture
    if(isCapture(move) && !isEnPassant(move)){
        board.pieces[getCapturedPiece(move)] ^= (1ULL<<to);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<<to);
        }
        else{
            board.blackPieces ^= (1ULL<<to);
        }
        board.occupied ^= (1ULL<<to);
        board.pieceOn[to]=(Piece) getCapturedPiece(move);
    }

    //promotion
    Piece promotionP=(Piece) getPromotionPiece(move);
    if(promotionP!=NO_PIECE){
        board.pieces[promotionP] ^= (1ULL<<to);
        board.pieces[getMovedPiece(move)] ^= (1ULL<<to);
    }

    //enpassant
    if(isEnPassant(move)){
        int pawnOn=(color==WHITE?to-8:to+8);
        board.pieces[getCapturedPiece(move)] ^= (1ULL<< pawnOn);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<< pawnOn);
        }else{
            board.blackPieces ^= (1ULL<< pawnOn);
        }
        board.occupied ^= (1ULL<<pawnOn);
        board.pieceOn[pawnOn]=(Piece) getCapturedPiece(move);
    }

    //castling
    if(isCastle(move)){
        Bitboard rookFromTo=0ULL;
        if(to==from+2){
            rookFromTo= (1ULL<< (to+1));
            rookFromTo |= (1ULL << (to -1));
            board.pieces[WR+myColOff] ^=rookFromTo;
            board.pieceOn[to-1]=NO_PIECE;
            board.pieceOn[to+1]=(Piece) (WR+myColOff);
        }else if(to==from-2){
            rookFromTo= (1ULL<< (to+1));
            rookFromTo |= (1ULL << (to-2));
            board.pieces[WR+myColOff] ^=rookFromTo;
            board.pieceOn[to+1]=NO_PIECE;
            board.pieceOn[to-2]=(Piece) (WR+myColOff);
        }
        if(color==WHITE){
            board.whitePieces ^=rookFromTo;
        }else{
            board.blackPieces ^=rookFromTo;
        }
        board.occupied ^=rookFromTo;
    }

    board.sideToMove ^= 1;
}
void makeNullMove(Board &board){
    // In makeNullMove()
    assert(board.ply >= 0 && board.ply < 512 && "makeNullMove: ply out of bounds");
    Undo& undo=board.history[board.ply++];
    undo.hash=board.hash;
    undo.enPassantSq=board.enPassantSquare;
    if(board.enPassantSquare!=NO_SQUARE){
        board.hash^=randomForEnPassant[board.enPassantSquare%8];
        board.enPassantSquare=NO_SQUARE;
    }
    board.hash^=randomForSide;
    board.sideToMove^=1;
}
void unmakeNullMove(Board &board){
    assert(board.ply > 0 && board.ply <= 512 && "unMake: ply underflow");
    Undo& undo=board.history[--board.ply];
    board.enPassantSquare=undo.enPassantSq;
    board.hash=undo.hash;
    board.sideToMove^=1;
}