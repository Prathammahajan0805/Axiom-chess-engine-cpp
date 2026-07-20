#include "makeMove.h"
#include "bitboards.h"
#include "board.h"
#include "moves.h"
using namespace std;



void makeMove(Board &board, Move &move){
    board.moveHistory[board.ply]=move;
    board.history[board.ply++]={board.enPassantSquare,board.whiteKingSquare,
        board.blackKingSquare,board.castlingRights};
    Color color=(Color) board.sideToMove;
    int myColOff=color*6;
    board.enPassantSquare=NO_SQUARE;

    //common
    
    int from=getFrom(move);
    int to=getTo(move);
    board.pieceOn[from]=NO_PIECE;
    board.pieceOn[to]=(Piece) getMovedPiece(move);
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

    //capture pieces
    if(isCapture(move) && !isEnPassant(move)){
        board.pieces[getCapturedPiece(move)] ^= (1ULL<<to);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<<to);
        }
        else{
            board.blackPieces ^= (1ULL<<to);
        }
        board.occupied ^= (1ULL<<to);
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
        if(getMovedPiece(move)==WP){
            board.enPassantSquare=to-8;
        }
        else board.enPassantSquare=to+8;
    }

    //promotion
    Piece promotionP=(Piece) getPromotionPiece(move);
    if(promotionP!=NO_PIECE){
        setBit(board.pieces[promotionP],to);
        board.pieces[getMovedPiece(move)] ^= (1ULL<<to);
        board.pieceOn[to]=promotionP;
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
        }else if(to==from-2){
            rookFromTo= (1ULL<< (to+1));
            rookFromTo |= (1ULL << (to-2));
            board.pieces[WR+myColOff] ^=rookFromTo;
            board.pieceOn[to-2]=NO_PIECE;
            board.pieceOn[to+1]=(Piece) (WR+myColOff);
        }
        if(color==WHITE){
            board.whitePieces ^=rookFromTo;
        }else{
            board.blackPieces ^=rookFromTo;
        }
        board.occupied ^=rookFromTo;
    }
    //remove castling rights
    if(getMovedPiece(move)==WK+myColOff){
        if(color==WHITE){
            board.whiteKingSquare=to;
            board.castlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
        }else{
            board.blackKingSquare=to;
            board.castlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
        }   
    }

    //remove castling rights for rook
    if(getMovedPiece(move)==WR+myColOff){
        switch (from) {
            case A1: board.castlingRights &= ~WHITE_QUEENSIDE; break;
            case H1: board.castlingRights &= ~WHITE_KINGSIDE; break;
            case A8: board.castlingRights &= ~BLACK_QUEENSIDE; break;
            case H8: board.castlingRights &= ~BLACK_KINGSIDE; break;
        }
    }
    
    board.sideToMove ^= 1;
}

void unMakeMove(Board &board){
    Move move=board.moveHistory[--board.ply];
    Undo undo=board.history[board.ply];

    //undo
    board.enPassantSquare=undo.enPassantSq;
    board.castlingRights=undo.castlingRights;
    board.blackKingSquare=undo.blackKingSq;
    board.whiteKingSquare=undo.whiteKingSq;

    Color color=(Color)( board.sideToMove ^ 1);
    int myColOff=color*6;
    Color opponentCol=(Color) (color ^ 1);
    int opponentOff=opponentCol*6;

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