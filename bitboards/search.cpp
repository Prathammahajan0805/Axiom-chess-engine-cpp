#include"search.h"
#include "board.h"
#include "evaluation.h"
#include "makeMove.h"
#include "movegen.h"
#include"moves.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cstdint>
constexpr int INF=32000;
constexpr int PROMOTION_PRIOR=10000;
namespace search_name {
    std::array<std::array<Move,2>,200> killer;
}
uint64_t nodes=0;
int getCaptureScore(const Move& move){
    int score=0;
    Piece moved=(Piece) (getMovedPiece(move)%6);
    Piece captured=(Piece) (getCapturedPiece(move)%6);
    score += (captured-moved+5)*1000;
    return score;
}
int scoreMove(const Move &move,int ply){
    bool isPromotion=getPromotionPiece(move)!=NO_PIECE;
    bool iscapture=isCapture(move);
    int score=0;
    if(isPromotion){
        score+=PROMOTION_PRIOR;
    }
    if(iscapture){
        score+=getCaptureScore(move);
    }
    if((search_name::killer[ply][0]).moveInfo==move.moveInfo){
        score+=6000;
    }
    if(search_name::killer[ply][1].moveInfo==move.moveInfo){
        score+=5000;
    }
    return score;
}
// int quiescence(Board &board,int alpha,int beta,int ply){
//     nodes++;
//     int standPat=eval(board);
//     if(standPat>=beta){
//         return beta;
//     }
//     if(standPat>alpha){
//         alpha=standPat;
//     }
// }
Move findBestMove(Board &board, int depth){
    MoveList moveList=generateMoves<ALL>(board);//get moves
    nodes=0;

    //initialise best move
    Move bestMove={0};

    //initialize killer moves
    search_name::killer.fill({0,0});

    //initialize best score
    int bestScore=INT_MIN;

    //checkmate detection
    if(moveList.count==0){
        int sq=(board.sideToMove==WHITE?board.whiteKingSquare:board.blackKingSquare);
        if(isSquareAttacked(sq,(Color) (board.sideToMove^1),board.occupied,board)){
            return (Move){0};
        }else{
            return (Move){1u << 31};
        }
    }

    //scores
    std::array<int,512> scores;
    for(int i=0;i<moveList.count;i++) scores[i]=scoreMove(moveList.moves[i],0);

    for(int i=0;i<moveList.count;i++){
        int maxScoreMove=scores[i],ind=i;

        //filtering for move scores
        for(int j=i;j<moveList.count;j++){
            if(maxScoreMove<scores[j]){
                maxScoreMove=scores[j];
                ind=j;
            }
        }

        //swap according to priorities
        std::swap(moveList.moves[i],moveList.moves[ind]);
        std::swap(scores[i],scores[ind]);  
        
        //find best move
        Move &move=moveList.moves[i];
        makeMove(board,move);

        //eval for next position
        int score=-search(board, depth-1,1,-INF,INF);

        //update scores and moves
        if(bestScore<score){
            bestScore=score;
            bestMove=move;
        }
        unMakeMove(board);
    }
    return bestMove;
}

int search(Board &board,int depth,int ply,int alpha,int beta){
    nodes++;
    if(depth==0) return eval(board);

    //generate moves
    MoveList moveList=generateMoves<ALL>(board);
    int bestScore=INT_MIN;

    //checkmate detection
    if(moveList.count==0){
        int sq=(board.sideToMove==WHITE?board.whiteKingSquare:board.blackKingSquare);
        if(isSquareAttacked(sq,(Color) (board.sideToMove^1),board.occupied,board)){
            return -MATE_SCORE+ply;
        }else{
            return DRAW_SCORE;
        }
    }

    //initialize scores
    std::array<int,256> scores;
    for(int i=0;i<moveList.count;i++) scores[i]=scoreMove(moveList.moves[i],ply);

    //search
    for(int i=0;i<moveList.count;i++){
        int maxScoreMove=scores[i],ind=i;

        //swap according to scores
        for(int j=i;j<moveList.count;j++){
            if(maxScoreMove<scores[j]){
                maxScoreMove=scores[j];
                ind=j;
            }
        }
        std::swap(moveList.moves[i],moveList.moves[ind]);
        std::swap(scores[i],scores[ind]);
        Move &move=moveList.moves[i];

        //evaluate
        makeMove(board,move);
        int score=-search(board, depth-1,ply+1,-beta,-alpha);
        if(score > bestScore)
            bestScore = score;

        if(score > alpha)
            alpha = score;
        unMakeMove(board);

        //alpha-beta pruning
        if(alpha>=beta){
            //update killer moves
            if(getPromotionPiece(move)==NO_PIECE && !isCapture(move) && search_name::killer[ply][0].moveInfo!=move.moveInfo){
                if(search_name::killer[ply][0].moveInfo==0){
                    search_name::killer[ply][0]=move;
                }else{
                    search_name::killer[ply][1]=search_name::killer[ply][0];
                    search_name::killer[ply][0]=move;
                }
            }
            break;
        }
    }
    
    return bestScore;
}
uint64_t getNodes(){ return nodes;}