#include"search.h"
#include "TT.h"
#include "board.h"
#include "evaluation.h"
#include "makeMove.h"
#include "movegen.h"
#include"moves.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include<chrono>
constexpr int INF=32000;
constexpr int PROMOTION_PRIOR=10000;
constexpr int TTMOVE_SCORE=100000;
constexpr int MAX_HISTORY=16000;
constexpr int MIN_HISTORY=-MAX_HISTORY;
namespace Hueristics {
    std::array<std::array<Move,2>,200> killer;
    std::array<std::array<std::array<int,64>,64>,2> historyHeuristic;
}
namespace timeControls{
    std::chrono::steady_clock::time_point startTime;
    int64_t timeLimitMs;
    bool stopSearch;
}
uint64_t nodes=0;
int getCaptureScore(const Move& move){
    int score=0;
    Piece moved=(Piece) (getMovedPiece(move)%6);
    Piece captured=(Piece) (getCapturedPiece(move)%6);
    score += (captured-moved+5)*1000;
    return score;
}
bool checkTime(){
    if((nodes&2047)!=0) return timeControls::stopSearch;

    auto now= std::chrono::steady_clock::now();
    auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-timeControls::startTime).count();

    if(elapsed>=timeControls::timeLimitMs){
        timeControls::stopSearch=true;
    }
    return timeControls::stopSearch;
}
void initSearch(){
    std::array<std::array<int,64>,64> arr1;
    std::array<int,64>arr2;
    arr2.fill(0);
    arr1.fill(arr2);
    Hueristics::historyHeuristic.fill(arr1);
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
    if((Hueristics::killer[ply][0]).moveInfo==move.moveInfo){
        score+=6000;
    }
    if(Hueristics::killer[ply][1].moveInfo==move.moveInfo){
        score+=5000;
    }
    score+=Hueristics::historyHeuristic[int(pieceColor(Piece(getMovedPiece(move))))][getFrom(move)][getTo(move)];
    return score;
}
int quiescence(Board &board,int alpha,int beta,int ply){
    nodes++;
    
    MoveList moveList=isInCheck((Color) board.sideToMove,board)?generateMoves<ALL>(board):generateMoves<CAPTURES>(board);
    bool inCheck=isInCheck((Color) board.sideToMove,board);
    if(!inCheck){
        int standPat=eval(board);
        if(standPat>=beta){
            return beta;
        }
        if(standPat>alpha){
            alpha=standPat;
        }
    }
    //get capture only moves
    if(moveList.count==0){
        if(inCheck){
            return -MATE_SCORE+ply;
        }else{
            return alpha;
        }
    }
    //get scores
    std::array<int,256> scores;
    for(int i=0;i<moveList.count;i++) scores[i]=getCaptureScore(moveList.moves[i]);

    //start quiescence search
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

        makeMove(board,moveList.moves[i]);
        int score=-quiescence(board,-beta,-alpha, ply+1);
        unMakeMove(board);
        if(score>=beta) return beta;
        if(score>alpha) alpha=score;
    }
    return alpha;
}
Move findBestMove(Board &board, int maxDepth,int64_t timeLimitMs){
    nodes=0;
    //time start
    timeControls::startTime=std::chrono::steady_clock::now();
    timeControls::timeLimitMs=timeLimitMs;
    timeControls::stopSearch=false;

    Move bestMove={0};
    int bestScore=0,score=0,alpha,beta;
    //iterative deepening
    for(int depth=1;depth<=maxDepth;depth++){
        Move rootBestMove={0};
        int window=25;
        while(true){
            if(depth==1){
                alpha=-INF;
                beta=INF;
            }else{
                alpha=score-window;
                beta=score+window;
            }
            score=search(board,depth, 0,alpha,beta,rootBestMove);
            if(timeControls::stopSearch){
                if(depth == 1 && bestMove.moveInfo == 0) bestMove=rootBestMove;
                break;
            }
            if(score<=alpha){
                alpha-=window;
                window*=2;
            }else if(score>=beta){
                beta+=window;
                window*=2;
            }else break;
        }
        bestMove=rootBestMove;
        bestScore=score;
    }
    return bestMove;
}
int search(Board &board,int depth,int ply,int alpha,int beta,Move& rootBestMove){
    nodes++;
    if(checkTime()) return 0;
    //quiescence search
    if(depth==0) return quiescence(board, alpha,  beta, ply);
    Move bestMove={0};
    //get ttmove
    TTEntry* entry=probe(board.hash);
    Move ttMove={0};
    if(entry){
        ttMove=entry->bestMove;
        if(entry->depth>=depth && ply!=0){
            if(entry->flag==EXACT) return entry->score;
            if(entry->flag==LOWERBOUND) alpha=std::max(alpha,entry->score);
            if(entry->flag==UPPERBOUND) beta=std::min(beta,entry->score);
            if(alpha>=beta) return entry->score;
        }
    }
    //generate moves
    MoveList moveList=generateMoves<ALL>(board);
    int bestScore=INT_MIN;

    //checkmate detection
    if(moveList.count==0){
        if(isInCheck((Color)board.sideToMove,board)){
            return -MATE_SCORE+ply;
        }else{
            return DRAW_SCORE;
        }
    }

    //initialize scores
    std::array<int,256> scores;
    for(int i=0;i<moveList.count;i++){ 
        if(moveList.moves[i].moveInfo==ttMove.moveInfo) scores[i]=TTMOVE_SCORE;
        else scores[i]=scoreMove(moveList.moves[i],ply);
    }
    bool isCheck=isInCheck((Color) board.sideToMove, board);

    //nullmove pruning
    if(depth>=3 && ply!=0 && !isCheck && hasNonePawnPieces(board) && beta<MATE_SCORE-1000){
        makeNullMove(board);
        int R=2+depth/4;
        int score=-search(board,  depth-1-R,  ply+1,-beta,-beta+1,rootBestMove);
        unmakeNullMove(board);
        if(timeControls::stopSearch) return 0;
        if(score>=beta) return beta;
    }

    //search
    int origAlpha=alpha;
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
        int score;
        if(i==0){
            score=-search(board, depth-1,ply+1,-beta,-alpha,rootBestMove);
            if(timeControls::stopSearch) return 0;
        }else{
            int newDepth=depth-1;
            bool neededScan=true;
            if(depth>=3 && i>=3 && getCapturedPiece(move)==NO_PIECE && !isCheck){
                int red=1+(i>6?1:0);
                int reducedDepth=std::max(newDepth-red,0);

                score=-search(board, reducedDepth,ply+1,-alpha-1,-alpha,rootBestMove);
                if(timeControls::stopSearch) return 0;
                neededScan=score>alpha;
            }
            if(neededScan){
                score=-search(board, depth-1,ply+1,-alpha-1,-alpha,rootBestMove);
                if(timeControls::stopSearch) return 0;
                if(score>alpha && score<beta){
                    score=-search(board, depth-1,ply+1,-beta,-alpha,rootBestMove);
                    if(timeControls::stopSearch) return 0;
                }
            }
        }
        unMakeMove(board);
        if(timeControls::stopSearch) return 0;
        if(score > bestScore){
            bestScore = score;
            bestMove=move;
        }
        if(score > alpha)
            alpha = score;
        //alpha-beta pruning
        if(alpha>=beta){
            //update history heuristic
            if(getPromotionPiece(move)==NO_PIECE && !isCapture(move)){
                int bonus=depth * depth;
                int from=getFrom(move),to=getTo(move);
                Hueristics::historyHeuristic[board.sideToMove][from][to]+=bonus-(Hueristics::historyHeuristic[board.sideToMove][from][to]*std::abs(bonus)/MAX_HISTORY);
            }
            //update killer moves
            if(getPromotionPiece(move)==NO_PIECE && !isCapture(move) && Hueristics::killer[ply][0].moveInfo!=move.moveInfo){
                if(Hueristics::killer[ply][0].moveInfo==0){
                    Hueristics::killer[ply][0]=move;
                }else{
                    Hueristics::killer[ply][1]=Hueristics::killer[ply][0];
                    Hueristics::killer[ply][0]=move;
                }
            }
            break;
        }
        
        //tt flag choosing
    }
        TTFlag flag;
        if(bestScore<=origAlpha){
            flag=UPPERBOUND;
        }
        else if(bestScore>=beta){
            flag=LOWERBOUND;
        }else flag=EXACT;

        //store in ttentry
        store(board.hash, bestMove,  depth, bestScore, flag);
        if(ply==0)rootBestMove=bestMove;
    
    return bestScore;
}
uint64_t getNodes(){ return nodes;}