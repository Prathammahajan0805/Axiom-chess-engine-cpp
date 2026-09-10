#include"see.h"
#include "attacks.h"
#include "bitboards.h"
#include "board.hpp"
#include "types.h"
#include <cmath>

Bitboard attackersTo(const Board& board,int square,Bitboard occ){
    Bitboard attackers=0ull;
    attackers |= (pawnAttacks[WHITE][square] & board.pieces[BP]) | (pawnAttacks[BLACK][square] & board.pieces[WP]);
    attackers |= (knightAttacks[square]) & (board.pieces[BN] | board.pieces[WN]);
    Bitboard bishopsAndQueens = board.pieces[WQ] | board.pieces[WB] | board.pieces[BQ] | board.pieces[BB];
    attackers |= bishopsAndQueens & getBishopAttacks(square,occ);
    Bitboard rooksAndQueens = board.pieces[WQ] | board.pieces[WR] | board.pieces[BQ] | board.pieces[BR];
    attackers |= getRookAttacks(square,occ) & rooksAndQueens;
    attackers |= kingAttacks[square] & (board.pieces[WK] | board.pieces[BK]);
    return attackers & occ;
}
int leastValuableAttacker(const Board& board, Bitboard attackers, int side, Piece& outPiece) {
    for (int type = 0; type <= 5; type++) {           
        Piece p = (Piece)(side * 6 + type);            
        Bitboard subset = attackers & board.pieces[p];
        if (subset) {
            outPiece = p;
            return bitScan(subset);
        }
    }
    return -1;
}
//static exchange evaluation
int see(const Board& board, const Move& move) {
    //get to and froms quares
    int toSq   = getTo(move);
    int fromSq = getFrom(move);
    //get captured and attackd pieces
    Piece captured = (Piece)(getCapturedPiece(move) % 6);
    Piece movedPiece=(Piece)(getMovedPiece(move));
    Piece attacker = (Piece)(movedPiece % 6);
    //iterative approach instead of recursive
    int gain[32];
    int depth = 0;

    //get initial attacker see val
    gain[0] = SEE_VALUE[captured];

    //make temp occ board
    Bitboard occ = board.occupied;

    //perform duped make move for occ board
    //remove fromsq
    occ &= ~(1ull<<fromSq);
    //if enpassant remove the to sq pawn
    if (isEnPassant(move)) {
        int sq=pieceColor(movedPiece)==WHITE?toSq-8:toSq+8;
        occ &= ~(1ull<<sq);
    }
    // marks side
    Color side = (Color)!board.sideToMove;
    Piece nextAttackerPiece = attacker;
    // get current attacker for the tosq
    Bitboard attackers = attackersTo(board, toSq,occ);

    while (true) {
        depth++;//increment depth

        // curr gain move(after opponent recaptures)
        gain[depth] = SEE_VALUE[nextAttackerPiece%6] - gain[depth - 1];

        //cutoff immediately if no side benifited from that trade we just wont trade then
        if (std::max(-gain[depth - 1], gain[depth]) < 0)
            break;

        int usedSq;
        //get lva sq
        int lva = leastValuableAttacker(board, attackers, side, nextAttackerPiece);
        if (lva == -1) break;//no attacker left
        usedSq = lva;//we capture with that piece
        occ &= ~(1ull<<usedSq);// reset the square from where the peice moved

        attackers = attackersTo(board, toSq,occ) & occ;// get the remaining attackers

        side = (Color)!side;//toggle side
        if(nextAttackerPiece%6==WK){
            if(attackers &( (nextAttackerPiece==WK)?board.blackPieces:board.whitePieces)) break;
        }
    }
    //let depth run out
    while (--depth > 0)
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);// get best gain you can get -ve side becouse of opposite side move
    //return best possible depth you can get
    return gain[0];
}
