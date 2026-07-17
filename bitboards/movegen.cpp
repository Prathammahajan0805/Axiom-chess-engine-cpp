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
        generateKingMoves(createMove, info,board);
        return createMove;
    }
    for(int i=start;i<=end;i++){
        switch (i%6) {
            case WP:generatePawnMoves( createMove,board, (Color) board.sideToMove,info); break;
            case WN:generateKnightMoves( createMove,info,board); break;
            case WK:generateKingMoves(createMove,info,board);break;
            default:generateSlidingPieces(createMove,(Piece) i,info,board);break;
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
            Piece p=board.pieceOn[to];
            if((1ULL<<to) & info.pinRay[sq]){
                createMove.moves[createMove.count++]=addMove(sq, to, WP+myColOffset,p,0,true);
            }
        }
        //promotion captures
        while(promotion){
            int to=popLSB(promotion);
            Piece p=board.pieceOn[to];
            if ((1ULL << to) & info.pinRay[sq]) {
            for(int i=WN+myColOffset;i<=WQ+myColOffset;i++){
                createMove.moves[createMove.count++]=addMove(sq, to, WP+myColOffset,p,i,true);
            }
            }  
        }
        //enpassants
        while(enpassants){
            int to=popLSB(enpassants);
            Piece p=board.pieceOn[color==WHITE?to-8:to+8];
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

void generateKnightMoves(MoveList &createMove, checkInfo &info,Board &board){
    Color color=(Color) board.sideToMove;
    int myColOff=color*6;
    Color opponentColor=color==WHITE?BLACK:WHITE;
    Bitboard knight=board.pieces[WN+myColOff];
    Bitboard myPieces=color==WHITE?board.whitePieces:board.blackPieces;
    Bitboard opponentPieces=color==WHITE?board.blackPieces:board.whitePieces;
    while(knight){
        int sq=popLSB(knight);
        
        if(info.pinned & (1ULL<< sq)) continue;

        Bitboard moves=knightAttacks[sq] & ~(myPieces);

        Bitboard silent=moves & ~(opponentPieces);
        Bitboard captures = moves & opponentPieces;

        silent &= info.checkMask;
        captures &= info.checkMask;

        //silent moves
        while(silent){
            int to=popLSB(silent);
            createMove.moves[createMove.count++]=addMove(sq, to,WN+myColOff);
        }

        //capture moves
        while(captures){
            int to = popLSB(captures);
            int capturedPiece=board.pieceOn[to];
            createMove.moves[createMove.count++]=addMove(sq, to,WN+myColOff,capturedPiece,0,true);
        }
    }
}

void generateKingMoves(MoveList &createMove, checkInfo &info,Board& board){
    Color color=(Color) board.sideToMove;
    Color attacker=(Color)(color^1);
    int myColOff=color*6;
    Bitboard ownPieces=color==WHITE?board.whitePieces:board.blackPieces;
    Bitboard enemyPieces=color==WHITE?board.blackPieces:board.whitePieces;
    Bitboard king=board.pieces[WK+myColOff];
    bool kingSide=(bool) (board.castlingRights & (color==WHITE?Castling::WHITE_KINGSIDE:Castling::BLACK_KINGSIDE));
    bool queenSide=(bool) (board.castlingRights & (color==WHITE?Castling::WHITE_QUEENSIDE:Castling::BLACK_QUEENSIDE));
    while(king){
        int sq=popLSB(king);
        Bitboard moves=kingAttacks[sq] & ~ownPieces;

        Bitboard silent=moves & ~(enemyPieces);
        Bitboard captures=moves & enemyPieces;

        //silent moves
        while(silent){
            int to =popLSB(silent);
            Bitboard occ = board.occupied;
            occ ^= (1ULL << sq);
            occ |= (1ULL << to);
            if (!isSquareAttacked(to, attacker, occ, board)){
                createMove.moves[createMove.count++]=addMove(sq, to,WK+myColOff);
            }
        }

        //capture moves
        while(captures){
            int to =popLSB(captures);
            int capturePiece=board.pieceOn[to];
            if(!isSquareAttacked(to, attacker,board.occupied^(1ULL<<sq),board)){
                createMove.moves[createMove.count++]=addMove(sq, to,WK+myColOff,capturePiece,0,true);
            }
        }
        if (isSquareAttacked(sq, attacker, board.occupied, board))
            return;
        if(kingSide && (board.pieceOn[sq+1]==NO_PIECE) && board.pieceOn[sq+2]==NO_PIECE && !isSquareAttacked(sq+1,attacker,board.occupied^(1ULL<<sq), board) && !isSquareAttacked(sq+2, attacker,board.occupied^(1ULL<<sq),board)){
            int to=sq+2;
            createMove.moves[createMove.count++]=addMove(sq, 
                to, 
                WK+myColOff,
                0,
                0,
                false,
                false,
                false,
                true);
        }
        if(queenSide && (board.pieceOn[sq-1]==NO_PIECE) && board.pieceOn[sq-2]==NO_PIECE && board.pieceOn[sq-3]==NO_PIECE && !isSquareAttacked(sq-1,attacker,board.occupied^(1ULL<<sq), board) && !isSquareAttacked(sq-2, attacker,board.occupied^(1ULL<<sq),board)){
            int to=sq-2;
            createMove.moves[createMove.count++]=addMove(sq, 
                to, 
                WK+myColOff,
                0,
                0,
                false,
                false,
                false,
                true);
        }
    }   
}

void generateSlidingPieces(MoveList &createMove, Piece piece, checkInfo &info,Board& board){
    Color color=(Color) board.sideToMove;
    int myOff=color*6;
    bool isRook = (piece==WR+myOff || piece==WQ+myOff);
    bool isBishop = (piece==WQ+myOff || piece==WB+myOff);
    Bitboard myPieces= color==WHITE? board.whitePieces:board.blackPieces;
    Bitboard enemyPieces=color==WHITE? board.blackPieces:board.whitePieces;
    if(isRook){
        Bitboard rook=!isBishop?board.pieces[WR+myOff]:board.pieces[WQ+myOff];
        while(rook){
            int sq=popLSB(rook);
            Bitboard moves=getRookAttacks(sq,board.occupied) & ~myPieces;

            moves &=info.checkMask;

            Bitboard silent=moves & ~enemyPieces;
            Bitboard captures=moves & enemyPieces;

            while(silent){
                int to=popLSB(silent);
                if((1ULL<<to) & info.pinRay[sq]){
                    createMove.moves[createMove.count++]=addMove(sq, to,piece);
                }
            }

            while(captures){
                int to = popLSB(captures);
                Piece capturePiece=board.pieceOn[to];
                if((1ULL<<to) & info.pinRay[sq]){
                    createMove.moves[createMove.count++]=addMove(sq, to,piece,capturePiece,0,true);
                }
            }
        }
    }
    if(isBishop){
        Bitboard bishop=!isRook?board.pieces[WB+myOff]:board.pieces[WQ+myOff];
        while(bishop){
            int sq=popLSB(bishop);
            Bitboard moves=getBishopAttacks(sq,board.occupied) & ~myPieces;

            moves &=info.checkMask;

            Bitboard silent=moves & ~enemyPieces;
            Bitboard captures=moves & enemyPieces;

            while(silent){
                int to=popLSB(silent);
                if((1ULL<<to) & info.pinRay[sq]){
                    createMove.moves[createMove.count++]=addMove(sq, to,piece);
                }
            }

            while(captures){
                int to = popLSB(captures);
                Piece capturePiece=board.pieceOn[to];
                if((1ULL<<to) & info.pinRay[sq]){
                    createMove.moves[createMove.count++]=addMove(sq, to,piece,capturePiece,0,true);
                }
            }
        }
    }
}

bool isSquareAttacked(int sq, Color attacker,Bitboard occupied, const Board& board) {
    int off = attacker * 6;
    Color mycol=(Color)(attacker^1);
    if (pawnAttacks[mycol][sq] & board.pieces[WP + off])
        return true;

    if (knightAttacks[sq] & board.pieces[WN + off])
        return true;

    if (kingAttacks[sq] & board.pieces[WK + off])
        return true;

    if (getRookAttacks(sq, occupied) &
        (board.pieces[WR + off] | board.pieces[WQ + off]))
        return true;

    if (getBishopAttacks(sq, occupied) &
        (board.pieces[WB + off] | board.pieces[WQ + off]))
        return true;

    return false;
}