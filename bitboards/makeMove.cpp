#include "makeMove.h"
#include "bitboards.h"
#include "board.h"
#include "moves.h"
using namespace std;

vector<Undo> history;
vector<Move> moveHistory;

void makeMove(Board &board, Move &move){
    moveHistory.push_back(move);
    history.push_back({board.enPassantSquare,board.whiteKingSquare,
        board.blackKingSquare,board.castlingRights});
    Color color=(Color) board.sideToMove;
    int myColOff=color*6;
    Color opponentColor=(Color)(color^1);
    int oppOff=opponentColor*6;

    //common
    int from=getFrom(move);
    int to=getTo(move);
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
    if(isCapture(move)){
        board.pieces[getCapturedPiece(move)] ^= (1ULL<<to);
        if(color==BLACK){
            board.whitePieces ^= (1ULL<<to);
        }
        else{
            board.blackPieces ^= (1ULL<<to);
        }
        board.occupied ^= (1ULL<<to);
    }
}

