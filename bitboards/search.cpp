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
constexpr int MAX_PLY=128;
std::array<std::array<Move,MAX_PLY>,MAX_PLY> pvTable;
std:: array<int,MAX_PLY> pvLength;
//dear devs this here is the heart of my engine and 3 to 4 months from my life if any of this seems over complicated or not needed that because it probably is not 
namespace Hueristics {
    //killer moves
    std::array<std::array<Move,2>,200> killer;
    //move history
    std::array<std::array<std::array<int,64>,64>,2> historyHeuristic;
    //continuation history
    std::array<std::array<std::array<std::array<int,64>,6>,64>,6> contHistory;
    std::array<std::pair<int,int>, 200> moveStack; 
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
//the least underwhelming function you will find here
void initSearch(){
    std::array<std::array<int,64>,64> arr1;
    std::array<int,64>arr2;
    arr2.fill(0);
    arr1.fill(arr2);
    Hueristics::historyHeuristic.fill(arr1);
    Move empty{0};
    std::array<std::array<int,64>,6> arr4;
    arr4.fill(arr2);
    std::array<std::array<std::array<int,64>,6>,64> arr3;
    arr3.fill(arr4);
    Hueristics::contHistory.fill(arr3);
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
    if(ply>0){
    int prevPiece=Hueristics::moveStack[ply-1].first, prevTo = Hueristics::moveStack[ply-1].second;
    score += Hueristics::contHistory[prevPiece][prevTo][getMovedPiece(move)%6][getTo(move)];
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
    for(int i=0;i<moveList.count;i++){
        if(!inCheck){
            scores[i]=getCaptureScore(moveList.moves[i],board);
        }else{
            scores[i]=Hueristics::historyHeuristic[pieceColor(Piece(getMovedPiece(moveList.moves[i])))][getFrom(moveList.moves[i])][getTo(moveList.moves[i])];
        }
    }

    //start quiescence search using selection sort to get nice move ordering and faster results 
    //why selction sort you ask because i can immediately select and use th move instead of sorting the whole thing and use it making overall less time consumming
    for(int i=0;i<moveList.count;i++){
        int maxScoreMove=scores[i],ind=i;

        //filtering for move scores (selection sort)
        for(int j=i;j<moveList.count;j++){
            if(maxScoreMove<scores[j]){
                maxScoreMove=scores[j];
                ind=j;
            }
        }

        //swap according to priorities
        std::swap(moveList.moves[i],moveList.moves[ind]);
        std::swap(scores[i],scores[ind]);

        //see pruning expensive as hell but works its charm so called once per capture here and for scoring moves at  getCaptureScore
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
            int newScore = search(board, depth, 0, alpha, beta,true);

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
        bestMove=pvTable[0][0];
        bestScore=score;
    }
    return bestMove;
}
int search(Board &board,int depth,int ply,int alpha,int beta,bool isPV){
    nodes++;
    if(checkTime() || (ply>0 && (isRepetition(board) || board.halfMoveClock>=100 ))) return 0;

    //if possition is way too bad that mate is incomming or way too god such that you are goin to mate there is nothing to search just return your alpha and be happy
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta  = std::min(beta,   MATE_SCORE - ply - 1);
    if(alpha >= beta) return alpha;

    //quiescence search check if beyond depth are their any capture that just blunders 
    if(depth==0) return quiescence(board, alpha,  beta, ply);

    //triangular pv array length of current ply
    pvLength[ply]=ply;

    Move bestMove={0};
    //get ttmove
    TTEntry* entry=probe(board.hash);
    Move ttMove={0};
    //check if any move is there
    if(entry){
        ttMove=entry->bestMove;
        if(entry->depth>=depth && ply!=0){
            if(entry->flag==EXACT) return scoreFromTT(entry->score, ply);//score is exactly this good
            if(entry->flag==LOWERBOUND) alpha=std::max(alpha,scoreFromTT(entry->score, ply));//score is atleast this good
            if(entry->flag==UPPERBOUND) beta=std::min(beta,scoreFromTT(entry->score, ply));//score is atmost this good
            if(alpha>=beta) return scoreFromTT(entry->score, ply);//direct cutoff no need to check
        }
    }

    
    bool isCheck=isInCheck((Color)board.sideToMove,board);
    //no tt move so we shave a depth off
    if(!entry && depth>=4 && !isCheck){
        depth -= 1;
    }
    //reverse futility pruning
    int staticEval = eval(board);
    if (!isCheck && depth <= 6 && ply != 0 && beta < MATE_SCORE - 1000) {
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
        int score=-search(board,  depth-1-R,  ply+1,-beta,-beta+1,false);
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

        //late move pruning/move count pruning this is to remove the backbenchers of bad move which recieve a bad score dont even let them try cause you will only waste time
        if(!isPV && !isCheck && depth<=8 && bestScore>-MATE_SCORE+1000 && getCapturedPiece(move)==NO_PIECE && getPromotionPiece(move)==NO_PIECE){
            int lmpThreshold = 3 + 2*depth*depth; // tune
            if(i >= lmpThreshold) continue;
        }
        //forward futiliy pruning bassically saying that the current position is so bad not even a good move can save me from this mess let alone a quiet one
        if(!isPV && !isCheck && depth<=8 && getCapturedPiece(move)==NO_PIECE && getPromotionPiece(move)==NO_PIECE && bestScore>-MATE_SCORE+1000){
            int futilityMargin = 100 + 90*depth; // tune
            if(staticEval + futilityMargin <= alpha) continue;
        }
        //see pruning nice right 
        if(!isPV && depth<=8 && bestScore>-MATE_SCORE+1000){
            int seeMargin = isCapture(move) ? -20*depth*depth : -50*depth; // tune both
            if(!seeGE(board, move, seeMargin)) continue;
        }
        //store move stack for continuation history
        Hueristics::moveStack[ply] = {getMovedPiece(move)%6, getTo(move)};

        //evaluate
        makeMove(board,move);
        int score;
        if(i==0){
            score=-search(board, newDepth,ply+1,-beta,-alpha,isPV);
        }else{
            bool neededScan=true;
            //LMR
            // so what lmr basicaly does is that it checks if the current move at lower depth atleast beats alhpa if it does only then full depth scan is needed else just skip that shit and move on
            bool givesCheck = isInCheck((Color)board.sideToMove, board); //sideToMove is now the opponent as toggled by make move
            if(depth>=3 && i>=3 && getCapturedPiece(move)==NO_PIECE && !isCheck && !givesCheck){
                int red = reductionTable[std::min(depth,63)][std::min(i,63)];
                int reducedDepth=std::max(newDepth-red,0);

                score=-search(board, reducedDepth,ply+1,-alpha-1,-alpha,false);
                neededScan=score>alpha;
            }
            //full scan this increases work if the score beats alpha but is an overall win if many node branches are pruned 
            if(neededScan){
                score=-search(board, newDepth,ply+1,-alpha-1,-alpha,false);
                if(score>alpha && score<beta){
                    score=-search(board, newDepth,ply+1,-beta,-alpha,isPV);

                }
            }
        }
        unMakeMove(board);

        // just time control stuff
        if(timeControls::stopSearch) return 0;
        if(score > bestScore){
            bestScore = score;
            bestMove=move;
        }
        if(score > alpha){
            alpha = score;
            pvTable[ply][ply]=move;
            for(int nextPly=ply+1;nextPly<pvLength[ply+1];nextPly++){
                pvTable[ply][nextPly]=pvTable[ply+1][nextPly];
            }
            pvLength[ply]=pvLength[ply+1];
        }
            
        //alpha-beta pruning(beta cut-off)
        if(alpha>=beta){
            //update history heuristic
            if(getPromotionPiece(move)==NO_PIECE && !isCapture(move)){
                int bonus=depth * depth;
                int from=getFrom(move),to=getTo(move);
                Hueristics::historyHeuristic[board.sideToMove][from][to]+=bonus-(Hueristics::historyHeuristic[board.sideToMove][from][to]*std::abs(bonus)/MAX_HISTORY);
                if(ply>0){
                    auto [prevPiece, prevTo] = Hueristics::moveStack[ply-1];
                    auto &ch = Hueristics::contHistory[prevPiece][prevTo][getMovedPiece(move)][to];
                    ch += bonus - (ch*std::abs(bonus)/MAX_HISTORY);
                }
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
                    int from=getFrom(prev), to=getTo(prev);
                    auto &h = Hueristics::historyHeuristic[board.sideToMove][from][to];
                    h += malus - (h * std::abs(malus) / MAX_HISTORY);
                    if(ply>0){
                        auto [prevPiece, prevTo] = Hueristics::moveStack[ply-1];
                        auto &ch = Hueristics::contHistory[prevPiece][prevTo][getMovedPiece(move)][to];
                        ch += malus - (ch * std::abs(malus) / MAX_HISTORY);
                    }
                }
            }
            break;
        }
        
    }
    //tt flag choosing
        if(timeControls::stopSearch) return 0;
        TTFlag flag;
        if(bestScore<=origAlpha){
            flag=UPPERBOUND;//atmost this good
        }
        else if(bestScore>=beta){
            flag=LOWERBOUND;//atleast this good
        }else flag=EXACT;//exactly this good

        //store in ttentry
        store(board.hash, bestMove,  depth, scoreToTT(bestScore, ply), flag);
    
    return bestScore;
}
uint64_t getNodes(){ return nodes;}