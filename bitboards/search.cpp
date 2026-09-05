#include"search.h"
#include "TT.h"
#include "board.h"
#include "evaluation.h"
#include "makeMove.h"
#include "movegen.h"
#include"moves.h"
#include "see.h"
#include "types.h"
#include "uci.h"
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
std::array<int,MAX_PLY> evalHistory;
struct searchTHreads{

};
bool isInsufficientMaterial(const Board& board)
{
    // Any pawn, rook or queen means mating material exists
    if (board.pieces[WP] | board.pieces[BP] | 
        board.pieces[WN] | board.pieces[BN] |
        board.pieces[WR] | board.pieces[BR] |
        board.pieces[WQ] | board.pieces[BQ])
    {
        return false;
    }

    int whiteKnights = popCount(board.pieces[WN]);
    int blackKnights = popCount(board.pieces[BN]);

    int whiteBishops = popCount(board.pieces[WB]);
    int blackBishops = popCount(board.pieces[BB]);

    int totalMinors =
        whiteKnights + blackKnights +
        whiteBishops + blackBishops;

    // K vs K
    if (totalMinors == 0)
        return true;

    // K+B vs K
    if (totalMinors == 1)
        return true;

    // K+N vs K
    if (totalMinors == 1)
        return true;

    // K+B vs K+B, same-colored bishops
    if (totalMinors == 2 &&
        whiteBishops == 1 &&
        blackBishops == 1)
    {
        int wb = bitScan(board.pieces[WB]);
        int bb = bitScan(board.pieces[BB]);

        return (wb%2)==(bb%2);
    }

    return false;
}
PawnHashTable pht;
//dear devs this here is the heart of my engine and 3 to 4 months from my life if any of this seems over complicated or not needed that because it probably is not 
namespace Hueristics {
    //killer moves
    std::array<std::array<Move,2>,200> killer;
    //move history
    std::array<std::array<std::array<int,64>,64>,2> historyHeuristic;
    //continuation history
    std::array<std::array<std::array<std::array<int,64>,6>,64>,6> contHistory;
    std::array<std::pair<int,int>, 200> moveStack; 
    //pawn correction
    std::array<std::array<int,16384>,2> pawnCorrHist{};
    constexpr int CORR_HISTORY_LIMIT = 16384;
    constexpr int CORR_SCALE = 256;
    std::array<std::array<std::array<int,6>,64>,6> captureHistory{};
    std::array<std::array<Move,64>,6> counterMove{}; // [prevMovedPiece][prevTo]
}                                                                                                                                                                                                                                                                                                                               //engine by none other than pratham mahajan
namespace timeControls{
    std::chrono::steady_clock::time_point startTime;
    int64_t timeLimitMs;
    int64_t optimalTimeMs;
    int64_t softLimitMs;
    bool stopSearch;
    
}
namespace timeManagement {
    int bestMoveStability;
    Move prevBestMove={0};
    std::array<int,MAX_PLY> scoreHistory;
    uint64_t rootBestMoveNodes=0;
    uint64_t rootTotalNodes=0;
    int rootBestScore = -INF;
    int rootSecondBestScore = -INF;
}
int correctedStaticEval(const Board& board, int rawEval){
    int corr = Hueristics::pawnCorrHist[board.sideToMove][board.pawnHash & (Hueristics::CORR_HISTORY_LIMIT-1)];
    int adjusted = rawEval + corr / Hueristics::CORR_SCALE;
    return std::clamp(adjusted, -MATE_SCORE+MAX_PLY, MATE_SCORE-MAX_PLY);
}

void updateCorrHist(const Board& board, int depth, int bestScore, int staticEval){
    int &entry = Hueristics::pawnCorrHist[board.sideToMove][board.pawnHash & (Hueristics::CORR_HISTORY_LIMIT-1)];
    int diff = (bestScore - staticEval) * Hueristics::CORR_SCALE;
    int weight = std::min(depth, 16);
    entry += (diff - entry) * weight / 32;
    entry = std::clamp(entry, -Hueristics::CORR_SCALE*128, Hueristics::CORR_SCALE*128);
}
int64_t computeSoftLimit(int depth){
    if( depth<=5) return timeControls::optimalTimeMs;

    //stobility factor
    float stabilityTable[]={1.50,1.2,1,0.9, 0.8,0.75};
    int64_t idx=std::min(timeManagement::bestMoveStability,5);
    float stabilityFactor=stabilityTable[idx];
    int64_t delta=timeManagement::scoreHistory[depth] - timeManagement::scoreHistory[depth-1];
    double scoreFactor;
    if(delta <= -40)      scoreFactor = 1.5;
    else if(delta <= -10) scoreFactor = 1.2;
    else if(delta >= 40)  scoreFactor = 1.1;
    else                  scoreFactor = 1.0;

    double nodesFactor = 1.0;
    if(timeManagement::rootTotalNodes > 0){
        double frac = (double)timeManagement::rootBestMoveNodes / (double)timeManagement::rootTotalNodes;
        nodesFactor = std::clamp((1.0 - frac) * 2.0, 0.5, 1.5);
    }
    double newSoft = timeControls::optimalTimeMs * stabilityFactor * scoreFactor * nodesFactor;
    return (int64_t)std::clamp(newSoft,timeControls::optimalTimeMs * 0.4,(double)timeControls::timeLimitMs);
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
    score += Hueristics::captureHistory[moved][getTo(move)][captured] / 4;
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
int captureValue(const Move& move){
    return SEE_VALUE[getCapturedPiece(move)%6]-SEE_VALUE[getMovedPiece(move)%6];
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
        if(Hueristics::counterMove[prevPiece][prevTo].moveInfo == move.moveInfo)
            score += 4000;
    }
    score+=Hueristics::historyHeuristic[int(pieceColor(Piece(getMovedPiece(move))))][getFrom(move)][getTo(move)];
    return score;
}
int quiescence(Board &board,int alpha,int beta,int ply){
    nodes++;
    if(ply >= MAX_PLY - 1) return eval(board);
    if(ply >= MAX_PLY - 1) return eval(board);
    if(checkTime()) return 0;
    MoveList moveList=isInCheck((Color) board.sideToMove,board)?generateMoves<ALL>(board):generateMoves<CAPTURES>(board);
    bool inCheck=isInCheck((Color) board.sideToMove,board);
    int standPat=eval(board);
    if(!inCheck){
        
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

        // delta pruning
        if(!inCheck && standPat + captureValue(moveList.moves[i]) + 200 < alpha) continue;

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
    timeControls::startTime=std::chrono::steady_clock::now();
    timeControls::timeLimitMs  = timeLimitMs;
    timeControls::optimalTimeMs = timeLimitMs;
    timeControls::softLimitMs  = timeLimitMs;
    timeControls::stopSearch=false;

    timeManagement::bestMoveStability = 0;
    timeManagement::prevBestMove = {0};
    timeManagement::scoreHistory.fill(0);
    int bestScore=0,score=0,alpha,beta;
    MoveList rootMoves=generateMoves<ALL>(board);
    Move bestMove={0};
    if (rootMoves.count > 0) bestMove = rootMoves.moves[0];
    else return Move{0};
    //iterative deepening
    for(int depth=1;depth<=maxDepth;depth++){
        timeManagement::rootBestScore = -INF;
        timeManagement::rootSecondBestScore = -INF;
        uint64_t nodesAtIterationStart = getNodes();
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

        if(bestMove.moveInfo == timeManagement::prevBestMove.moveInfo){
            timeManagement::bestMoveStability = std::min(timeManagement::bestMoveStability + 1, 10);
        } else {
            timeManagement::bestMoveStability = 0;
        }
        timeManagement::prevBestMove = bestMove;
        timeManagement::scoreHistory[depth] = bestScore;
        timeManagement::rootTotalNodes = getNodes() - nodesAtIterationStart;

        timeControls::softLimitMs = computeSoftLimit(depth);

        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - timeControls::startTime).count();
        uint64_t nps = elapsedMs > 0 ? (getNodes() * 1000) / elapsedMs : 0;

        std::string scoreStr;
        if (bestScore >= MATE_SCORE - 1000) {
            int mateIn = (MATE_SCORE - bestScore + 1) / 2;
            scoreStr = "mate " + std::to_string(mateIn);
        } else if (bestScore <= -MATE_SCORE + 1000) {
            int mateIn = (-MATE_SCORE - bestScore) / 2; // will be negative
            scoreStr = "mate " + std::to_string(mateIn);
        } else {
            scoreStr = "cp " + std::to_string(bestScore);
        }

        std::cout << "info depth " << depth
                << " score " << scoreStr
                << " nodes " << getNodes()
                << " nps " << nps
                << " time " << elapsedMs
                << " pv";
        for (int i = 0; i < pvLength[0]; i++)
            std::cout << " " << moveToUci(pvTable[0][i]);
        std::cout << std::endl;
        bool easyMove= depth>=6 && timeManagement::bestMoveStability>=4 && (timeManagement::rootBestScore - timeManagement::rootSecondBestScore) > 50;

        if(elapsedMs >= timeControls::softLimitMs){
            break;
        }

        if(easyMove && elapsedMs >= timeControls::optimalTimeMs*0.3) break;
    }
    std::string output = "bestmove " + moveToUci(bestMove);
    if (pvLength[0] >= 2) {
        output += " ponder " + moveToUci(pvTable[0][1]);
    }
    std::cout << output << std::endl;
    return bestMove;
}
int search(Board &board,int depth,int ply,int alpha,int beta,bool isPV, Move excludedMove ){
    nodes++;
    if(ply >= MAX_PLY - 1) return eval(board);
    if(checkTime() || (ply>0 && (isRepetition(board) || board.halfMoveClock>=100 )) || isInsufficientMaterial(board)) return 0;
    if(ply >= MAX_PLY - 1) return eval(board);
    //if possition is way too bad that mate is incomming or way too god such that you are goin to mate there is nothing to search just return your alpha and be happy
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta  = std::min(beta,   MATE_SCORE - ply - 1);
    if(alpha >= beta) return alpha;

    bool isCheck=isInCheck((Color)board.sideToMove,board);
    
    //probCuting

    if(depth>=5 && !isPV && !isCheck){ 
        int pcBeta=beta+20*depth;
        int s=search(board, depth-4, ply, pcBeta-1, pcBeta,false); 
        if(s>=pcBeta) return s; 
    }

    //quiescence search check if beyond depth are their any capture that just blunders 
    if(depth==0) return quiescence(board, alpha,  beta, ply);

    //triangular pv array length of current ply
    pvLength[ply]=ply;

    Move bestMove={0};
    //get ttmove
    TTEntry* entry=probe(board.hash);
    bool singularCandidate = entry && !excludedMove.moveInfo && ply>0 && depth>=8
    && entry->depth >= depth-3 && entry->flag != UPPERBOUND
    && std::abs(entry->score) < MATE_SCORE-1000;

    int extension = 0;
    Move ttMove={0};
    if(singularCandidate){
        int sBeta = scoreFromTT(entry->score, ply) - 2*depth;
        int sDepth = (depth-1)/2;
        int sScore = search(board, sDepth, ply, sBeta-1, sBeta, false, ttMove);
        if(sScore < sBeta){
            extension = 1; // ttMove is singular extend it
        } else if(sBeta >= beta){
            return sBeta; // multi-cut: even a "worse" move than ttMove beats beta prune whole node
        }if(sScore>= sBeta) extension -=1;
        
    }
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

    
    //no tt move so we shave a depth off
    if(!entry && depth>=4 && !isCheck){
        depth -= 1;
    }
    
    int rawEval = eval(board);
    int staticEval = correctedStaticEval(board, rawEval);
    evalHistory[ply] = staticEval;
    // is the position improving
    bool improving = (ply >= 2 && staticEval > evalHistory[ply-2]);
    
    //reverse futility pruning
    if (!isCheck && depth <= 6 && ply != 0 && beta < MATE_SCORE - 1000) {
        int baseMargin = 120 * depth; // tune this
        int margin=baseMargin-(improving?0:30);
        if (staticEval - margin >= beta)
            return staticEval - margin;
    }

    //razoring- at very low depth  with a static  eval well below alpha send straight to quiescence
    if(!isCheck && depth<=3 && staticEval+depth*120<alpha ){
        int qScore=quiescence(board,alpha,beta,ply);
        if(qScore<alpha) return qScore;
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
        if(excludedMove.moveInfo && move.moveInfo==excludedMove.moveInfo) continue;
        int thisNewDepth = newDepth + (move.moveInfo==ttMove.moveInfo ? extension : 0);
        //late move pruning/move count pruning this is to remove the backbenchers of bad move which recieve a bad score dont even let them try cause you will only waste time
        if(!isPV && !isCheck && depth<=8 && bestScore>-MATE_SCORE+1000 && getCapturedPiece(move)==NO_PIECE && getPromotionPiece(move)==NO_PIECE){
            int lmpThreshold = 3 + 2*depth*depth; // tune
            if(i >= lmpThreshold) continue;
        }
        //forward futiliy pruning bassically saying that the current position is so bad not even a good move can save me from this mess let alone a quiet one
        if(!isPV && !isCheck && depth<=8 && getCapturedPiece(move)==NO_PIECE && getPromotionPiece(move)==NO_PIECE && bestScore>-MATE_SCORE+1000){
            int futilityMargin = 100 + 90*depth; // tune
            futilityMargin-= (improving?0:30);//tune
            if(staticEval + futilityMargin <= alpha) continue;
        }
        //see pruning nice right 
        if(!isPV && depth<=8 && bestScore>-MATE_SCORE+1000){
            int seeMargin = isCapture(move) ? -20*depth*depth : -50*depth; // tune both
            seeMargin-=improving?0:30;
            if(!seeGE(board, move, seeMargin)) continue;
        }
        //store move stack for continuation history
        Hueristics::moveStack[ply] = {getMovedPiece(move)%6, getTo(move)};
        uint64_t nodesBeforeThisMove = nodes;

        //evaluate
        makeMove(board,move);
        int score;
        if(i==0){
            score=-search(board, thisNewDepth,ply+1,-beta,-alpha,isPV);
        }else{
            bool neededScan=true;

            //LMR
            // so what lmr basicaly does is that it checks if the current move at lower depth atleast beats alhpa if it does only then full depth scan is needed else just skip that shit and move on
            bool givesCheck = isInCheck((Color)board.sideToMove, board); //sideToMove is now the opponent as toggled by make move
            if(depth>=3 && i>=3 && getCapturedPiece(move)==NO_PIECE && !isCheck && !givesCheck){
                int red = reductionTable[std::min(depth,63)][std::min(i,63)];
                int histScore = Hueristics::historyHeuristic[board.sideToMove][getFrom(move)][getTo(move)];
                red -= histScore / 8192;
                red = std::clamp(red, 0, newDepth);
                if(!improving) red+=1;
                int reducedDepth=std::max(thisNewDepth-red,0);

                score=-search(board, reducedDepth,ply+1,-alpha-1,-alpha,false);
                neededScan=score>alpha;
            }
            //full scan this increases work if the score beats alpha but is an overall win if many node branches are pruned 
            if(neededScan){
                score=-search(board, thisNewDepth,ply+1,-alpha-1,-alpha,false);
                if(score>alpha && score<beta){
                    score=-search(board, thisNewDepth,ply+1,-beta,-alpha,isPV);

                }
            }
        }
        unMakeMove(board);
        if(ply==0 && score > bestScore){
            timeManagement::rootBestMoveNodes = nodes - nodesBeforeThisMove;
        }
        if(ply == 0){
            if(score > timeManagement::rootBestScore){
                timeManagement::rootSecondBestScore = timeManagement::rootBestScore;
                timeManagement::rootBestScore = score;
            } else if(score > timeManagement::rootSecondBestScore){
                timeManagement::rootSecondBestScore = score;
            }
        }
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
                    auto &ch = Hueristics::contHistory[prevPiece][prevTo][getMovedPiece(move)%6][to];
                    ch += bonus - (ch*std::abs(bonus)/MAX_HISTORY);
                }
            }
            //cpature history
            if(isCapture(move)){
                Piece moved=(Piece)(getMovedPiece(move)%6), captured=(Piece)(getCapturedPiece(move)%6);
                int bonus=depth*depth;
                auto &ch = Hueristics::captureHistory[moved][getTo(move)][captured];
                ch += bonus - (ch*std::abs(bonus)/MAX_HISTORY);
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
            if(ply>0 && getPromotionPiece(move)==NO_PIECE && !isCapture(move)){
                auto [prevPiece, prevTo] = Hueristics::moveStack[ply-1];
                Hueristics::counterMove[prevPiece][prevTo] = move;
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
                        auto &ch = Hueristics::contHistory[prevPiece][prevTo][getMovedPiece(prev)%6][to];
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
        if(!isCheck && getPromotionPiece(bestMove)==NO_PIECE)
        updateCorrHist(board, depth, bestScore, staticEval);
        //store in ttentry
        if(!excludedMove.moveInfo){
            store(board.hash, bestMove,  depth, scoreToTT(bestScore, ply), flag);
        }
    
    return bestScore;
}
uint64_t getNodes(){ return nodes;}