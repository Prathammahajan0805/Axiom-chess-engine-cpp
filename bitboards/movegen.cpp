#include<iostream>
#include"movegen.h"
#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "moves.h"


//generate all moves
MoveList generateMoves(Board &board){
    MoveList createMove;
    int start=board.sideToMove==WHITE? WP:BP;
    int end=board.sideToMove==WHITE?WK:BK;
    checkInfo info;
    generateCheckersAndPins(board,info);
    if(popCount(info.checkers)>=2){
        generateKingMoves(board.sideToMove==WHITE?board.whiteKingSquare:board.blackKingSquare,createMove, info);
        return createMove;
    }
    for(int i=start;i<=end;i++){
        Bitboard piece=board.pieces[i];
        while(piece){
            int sq=popLSB(piece);
            switch (i%6) {
                case WP: generatePawnMoves( createMove,board, (Color) board.sideToMove,info); break;
                case WN:generateKnightMoves( sq, createMove,info); break;
                case WK:generateKingMoves(sq,createMove,info);break;
                default:generateSlidingPieces(sq,createMove,(Piece) i,info);break;
            }
        }
    }
    return createMove;
}

//generate checkers and pins
void generateCheckersAndPins(Board &board, checkInfo &info){
    int sq=board.sideToMove==WHITE?board.whiteKingSquare:board.blackKingSquare;
    Bitboard ownPieces=board.sideToMove==WHITE?board.whitePieces:board.blackPieces;
    Color opponentColor=board.sideToMove==WHITE?BLACK:WHITE;
    int offset=opponentColor*6;
    info.checkers=0ULL;
    info.checkMask=0ULL;
    info.pinned=0ULL;
    for(int i=0;i<64;i++) info.pinRay[i]= ~(0ULL);
    //-------------------------------------check calc--------------------------------
    //pawns
    
    info.checkers |= pawnAttacks[board.sideToMove][sq] & board.pieces[WP+offset];
    
    //knights
    
    info.checkers |= knightAttacks[sq] & board.pieces[WN +offset];

    //kings

    info.checkers |= kingAttacks[sq] & board.pieces[WK + offset];
    
    //rooks and queens
    
    Bitboard opponentRooksAndQueen=board.pieces[WR+offset] | board.pieces[WQ+offset];
    Bitboard attacks=getRookAttacks( sq, board.occupied);
    info.checkers |= attacks & opponentRooksAndQueen;

    //bishops and queens
    Bitboard opponentBishopAndQueen=board.pieces[WB+offset] | board.pieces[WQ+offset];
    attacks=getBishopAttacks(sq,board.occupied);
    info.checkers |= attacks & opponentBishopAndQueen;

    //make a check mask
    int numCheckers=popCount(info.checkers);
    if(numCheckers==0){
        info.checkMask= ~(0ULL);
    }else if(numCheckers==1){
        int attackerSq=bitScan(info.checkers);
        info.checkMask=between[sq][attackerSq] | (1ULL<<attackerSq);
    }else{
        info.checkMask=0ULL;
    }

    //----------------------calc pins------------------------------
    Bitboard sliders= getRookAttacks(sq,0ULL) & opponentRooksAndQueen;
    while(sliders){
        int attackerSq=popLSB(sliders);
        Bitboard blockers=between[sq][attackerSq] & board.occupied;
        if(popCount(blockers)==1 && (blockers & ownPieces)){
            int pinnedSq=bitScan(blockers);
            info.pinned |=blockers;
            info.pinRay[pinnedSq]=between[sq][attackerSq] | (1ULL<<attackerSq);
        }
    }

    sliders=getBishopAttacks(sq,0ULL) & opponentBishopAndQueen;
    
    while(sliders){
        int attackerSq=popLSB(sliders);
        Bitboard blockers=between[sq][attackerSq] & board.occupied;
        if(popCount(blockers)==1 && (blockers & ownPieces)){
            int pinnedSq=bitScan(blockers);
            info.pinned |=blockers;
            info.pinRay[pinnedSq]=between[sq][attackerSq] | (1ULL<<attackerSq);
        }
    }
}

void generatePawnMoves(MoveList &createMove, Board &board, Color color, checkInfo &info){
    Color opponentColor=color==WHITE?BLACK:WHITE;
    Bitboard opponents = color==WHITE? board.blackPieces:board.whitePieces;
    int myColOffset=color*6;
    int offset=opponentColor*6;
    int myKingSq=color==WHITE? board.whiteKingSquare:board.blackKingSquare;

    Bitboard enemyRooksQueens = board.pieces[WR + offset] | board.pieces[WQ + offset];
    Bitboard enemyBishopsQueens = board.pieces[WB + offset] | board.pieces[WQ + offset];
    //single push
    Bitboard single = ((color==WHITE )?
            board.pieces[WP+myColOffset]<<8
            :board.pieces[myColOffset+WP]>>8) & ~(board.occupied);

    
    //double push
    Bitboard dblPush=((color==WHITE)?
            (single & RANK_3) << 8
            : (single & RANK_6)>>8) & ~(board.occupied);

    single &= info.checkMask;
    Bitboard promotionRank = (color == WHITE) ? RANK_8 : RANK_1;
    Bitboard promotion = single & promotionRank;
    single &= ~promotionRank;
    dblPush &=info.checkMask;

    while(single){
        int to=popLSB(single);
        int from= color==WHITE? to-8:to+8;
        if((1ULL<<to )& info.pinRay[from]){
            createMove.moves[createMove.count++]=addMove(from, to,WP+myColOffset);
        }
    }
    while(dblPush){
        int to=popLSB(dblPush);
        int from= color==WHITE? to-16:to+16;
        if((1ULL<<to )& info.pinRay[from]){
            createMove.moves[createMove.count++]=addMove(from, to,WP+myColOffset,0,0,false,true);
        }
    }
    while(promotion){
        int to=popLSB(promotion);
        int from=color==WHITE?to-8:to+8;
        if ((1ULL << to) & info.pinRay[from]) {
        for(int i=WN+myColOffset;i<=WQ+myColOffset;i++){
            createMove.moves[createMove.count++]=addMove(from, to, WP+myColOffset,0,i);
        }
        }   
    }

    //capturing moves
    Bitboard pawns=board.pieces[WP+myColOffset];
    
    while(pawns){
        Bitboard enpassants=0ULL;
        int sq=popLSB(pawns);
        Bitboard attacks = pawnAttacks[color][sq] & opponents;
        if(board.enPassantSquare!=NO_SQUARE) enpassants = pawnAttacks[color][sq] & (1ULL<<board.enPassantSquare);
        attacks &=info.checkMask;
        promotion=attacks & promotionRank;
        attacks &= ~promotionRank;
        enpassants &=info.checkMask;
        //normal attacks
        while(attacks){
            int to=popLSB(attacks);
            Piece p=board.findPiece(opponentColor,to);
            if((1ULL<<to) & info.pinRay[sq]){
                createMove.moves[createMove.count++]=addMove(sq, to, WP+myColOffset,p,0,true);
            }
        }
        //promotion captures
        while(promotion){
            int to=popLSB(promotion);
            Piece p=board.findPiece(opponentColor,to);
            if ((1ULL << to) & info.pinRay[sq]) {
            for(int i=WN+myColOffset;i<=WQ+myColOffset;i++){
                createMove.moves[createMove.count++]=addMove(sq, to, WP+myColOffset,p,i,true);
            }
            }  
        }
        //enpassants
        while(enpassants){
            int to=popLSB(enpassants);
            Piece p=board.findPiece(opponentColor,color==WHITE?to-8:to+8);
            Bitboard occ=board.occupied;
            occ^=(1ULL<<sq);
            occ|=(1ULL<<to);
            int capturedSq = (color == WHITE) ? to - 8 : to + 8;
            occ ^= (1ULL << capturedSq);
            if((getRookAttacks(myKingSq,occ) & enemyRooksQueens) || 
            (getBishopAttacks(myKingSq, occ) & enemyBishopsQueens)) continue;
            if(info.pinRay[sq]&(1ULL<<to)) createMove.moves[createMove.count++]=addMove(sq, to,WP+myColOffset,p,0,true,false,true);
        }
        
    }
    
}
