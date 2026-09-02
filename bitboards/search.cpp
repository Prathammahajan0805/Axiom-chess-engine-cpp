#include"search.h"
#include "TT.h"
#include "board.h"
#include "evaluation.h"
#include "makeMove.h"
#include "movegen.h"
#include"moves.h"
#include "see.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include<chrono>
#include<cmath>
constexpr int INF=32000;
constexpr int PROMOTION_PRIOR=10000;
constexpr int TTMOVE_SCORE=100000;
constexpr int MAX_HISTORY=16000;
constexpr int MIN_HISTORY=-MAX_HISTORY;
//dear devs this here is the heart of my engine and 3 to 4 months from my life if any of this seems over complicated or not needed that because it probably is not 
namespace Hueristics {
    std::array<std::array<Move,2>,200> killer;
    std::array<std::array<std::array<int,64>,64>,2> historyHeuristic;
}                                                                                                                                                                                                                                                                                                                               //engine by none other than pratham mahajan
namespace timeControls{
    std::chrono::steady_clock::time_point startTime;
    int64_t timeLimitMs;
    bool stopSearch;
}
void stopSearch(){
    timeControls::stopSearch=true;
}
void clearKillers(){
    Move empty={0};
    std::array<Move,2> emptyKillers{empty, empty};
    Hueristics::killer.fill(emptyKillers);
}
uint64_t nodes=0;
std::array<std::array<int,64>,64> reductionTable;
//this here is taken straight from reliable sources (chess programming wiki) dont ask me why these constants or curve it is the most tested and trusted one i have found so bare with me
void initLMR() {
    for (int d = 1; d < 64; d++)
        for (int m = 1; m < 64; m++)
            reductionTable[d][m] = (int)(0.75 + log(d) * log(m) / 2.25); // to tune
}
int scoreToTT(int score, int ply) {
    if (score >= MATE_SCORE - 1000) return score + ply;
    if (score <= -MATE_SCORE + 1000) return score - ply;
    return score;
}
int scoreFromTT(int score, int ply) {
    if (score >= MATE_SCORE - 1000) return score - ply;
    if (score <= -MATE_SCORE + 1000) return score + ply;
    return score;
}
int getCaptureScore(const Move& move,const Board& board){
    int score=0;
    Piece moved=(Piece) (getMovedPiece(move)%6);
    Piece captured=(Piece) (getCapturedPiece(move)%6);
    score += (captured-moved+5)*1000;
    if(see(board,move)<0) score-=30000;
    else score+=20000;
    return score;
}
bool isRepetition(const Board& board){
    for(int i=board.ply-2;i>=std::max(0,board.ply-board.halfMoveClock);i-=2){
        if(board.hash==board.hashHistory[i]) return true;
    }
    return false;
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
    Move empty{0};

    std::array<Move,2> emptyKillers{empty, empty};

    Hueristics::killer.fill(emptyKillers);
    clearTT();

}
int scoreMove(const Move &move,int ply,const Board& board){
    bool isPromotion=getPromotionPiece(move)!=NO_PIECE;
    bool iscapture=isCapture(move);
    int score=0;
    if(isPromotion){
        score+=PROMOTION_PRIOR;
    }
    if(iscapture){
        score+=getCaptureScore(move,board);
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
    if(checkTime()) return 0;
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
    for(int i=0;i<moveList.count;i++) scores[i]=getCaptureScore(moveList.moves[i],board);

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

        //see pruning
         if (!inCheck && isCapture(moveList.moves[i]) && !seeGE(board, moveList.moves[i], 0))
            continue;

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

    // clear killers
    clearKillers();
    timeControls::startTime=std::chrono::steady_clock::now();
    timeControls::timeLimitMs=timeLimitMs;
    timeControls::stopSearch=false;

    int bestScore=0,score=0,alpha,beta;
    MoveList rootMoves=generateMoves<ALL>(board);
    Move bestMove={0};
    if (rootMoves.count > 0) bestMove = rootMoves.moves[0];
    else return Move{0};
    //iterative deepening
    for(int depth=1;depth<=maxDepth;depth++){
        Move rootBestMove=bestMove;
        int alphaWindow=25, betaWindow=25;
        alpha = score - alphaWindow;
        beta  = score + betaWindow;
        //aspiration windows
        while(true) {
            int newScore = search(board, depth, 0, alpha, beta, rootBestMove);

            if(timeControls::stopSearch)
                break;

            if(newScore <= alpha) {
                alphaWindow *= 2;
                alpha = score - alphaWindow;
            } else if(newScore >= beta) {
                betaWindow *= 2;
                beta = score + betaWindow;
            } else {
                score = newScore;
                break;
            }
        }

        if(timeControls::stopSearch){
            break;
        }
        bestMove=rootBestMove;
        bestScore=score;
    }
    return bestMove;
}
int search(Board &board,int depth,int ply,int alpha,int beta,Move& rootBestMove){
    nodes++;
    if(checkTime() || (ply>0 && (isRepetition(board) || board.halfMoveClock>=100 ))) return 0;
    //quiescence search check if beyond depth are their any capture that just blunders 
    if(depth==0) return quiescence(board, alpha,  beta, ply);
    Move bestMove={0};
    //get ttmove
    TTEntry* entry=probe(board.hash);
    Move ttMove={0};
    if(entry){
        ttMove=entry->bestMove;
        if(entry->depth>=depth && ply!=0){
            if(entry->flag==EXACT) return scoreFromTT(entry->score, ply);
            if(entry->flag==LOWERBOUND) alpha=std::max(alpha,scoreFromTT(entry->score, ply));
            if(entry->flag==UPPERBOUND) beta=std::min(beta,scoreFromTT(entry->score, ply));
            if(alpha>=beta) return scoreFromTT(entry->score, ply);
        }
    }
    bool isCheck=isInCheck((Color)board.sideToMove,board);

    //reverse futility pruning
    if (!isCheck && depth <= 6 && ply != 0 && beta < MATE_SCORE - 1000) {
        int staticEval = eval(board);
        int margin = 120 * depth; // tune this
        if (staticEval - margin >= beta)
            return staticEval - margin;
    }
    //generate moves
    MoveList moveList=generateMoves<ALL>(board);
    int bestScore=-INF;

    //checkmate detection
    if(moveList.count==0){
        if(isCheck){
            return -MATE_SCORE+ply;
        }else{
            return DRAW_SCORE;
        }
    }

    //initialize scores
    std::array<int,256> scores;
    for(int i=0;i<moveList.count;i++){ 
        if(moveList.moves[i].moveInfo==ttMove.moveInfo) scores[i]=TTMOVE_SCORE;
        else scores[i]=scoreMove(moveList.moves[i],ply,board);
    }

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
    int newDepth = depth-1 + (isCheck ? 1 : 0);
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
            score=-search(board, newDepth,ply+1,-beta,-alpha,rootBestMove);
        }else{
            bool neededScan=true;
            //LMR
            // so what lmr basicaly does is that it checks if the current move at lower depth atleast beats alhpa if it does only then full depth scan is needed else just skip that shit and move on
            if(depth>=3 && i>=3 && getCapturedPiece(move)==NO_PIECE && !isCheck){
                int red = reductionTable[std::min(depth,63)][std::min(i,63)];
                int reducedDepth=std::max(newDepth-red,0);

                score=-search(board, reducedDepth,ply+1,-alpha-1,-alpha,rootBestMove);
                neededScan=score>alpha;
            }
            //full scan this increases work if the score beats alpha but is an overall win if many node branches are pruned 
            if(neededScan){
                score=-search(board, newDepth,ply+1,-alpha-1,-alpha,rootBestMove);
                if(score>alpha && score<beta){
                    score=-search(board, newDepth,ply+1,-beta,-alpha,rootBestMove);

                }
            }
        }
        unMakeMove(board);

        // just time control stuff
        if(timeControls::stopSearch) return 0;
        if(score > bestScore){
            bestScore = score;
            bestMove=move;
            if(ply == 0) rootBestMove = bestMove;
        }
        if(score > alpha)
            alpha = score;
        //alpha-beta pruning(beta cut-off)
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
            // penalize a move that was tried but failed (malus)
            for (int j = 0; j < i; j++) {
                Move &prev = moveList.moves[j];
                if (getPromotionPiece(prev)==NO_PIECE && !isCapture(prev)) {
                    int malus = -depth*depth;
                    int f=getFrom(prev), t=getTo(prev);
                    auto &h = Hueristics::historyHeuristic[board.sideToMove][f][t];
                    h += malus - (h * std::abs(malus) / MAX_HISTORY);
                }
            }
            break;
        }
        
    }
    //tt flag choosing
        if(timeControls::stopSearch) return 0;
        TTFlag flag;
        if(bestScore<=origAlpha){
            flag=UPPERBOUND;
        }
        else if(bestScore>=beta){
            flag=LOWERBOUND;
        }else flag=EXACT;

        //store in ttentry
        store(board.hash, bestMove,  depth, scoreToTT(bestScore, ply), flag);
        if(ply==0)rootBestMove=bestMove;
    
    return bestScore;
}
uint64_t getNodes(){ return nodes;}