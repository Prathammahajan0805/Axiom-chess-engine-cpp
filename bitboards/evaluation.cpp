#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include"evalualtion.h"
#include "types.h"
#include<array>
std::array<const int,12> gamephaseInc={0,1,1,2,4,0,0,1,1,2,4,0};
namespace pesto{
static std::array<const int*,6> middlegame_Tables={
    middlegamePawn_Table,
    middlegameKnight_table,
    middlegameBishop_table,
    middlegameRook_table,
    middlegameQueen_table,
    middlegameKing_table
};
static std::array<const int*,6> endgame_Tables={
    endgamePawn_Table,
    endgameKnight_table,
    endgameBishop_table,
    endgameRook_table,
    endgameQueen_table,
    endgameKing_table
};
static const int bishopPairBonusMiddlegame=20;
static const int bishopPairBonusEndgame=40;
constexpr int mg_passedPawnBonus[6]={0,10,20,40,70,120};
constexpr int eg_passedPawnBonus[6]={0,20,40,70,120,200};
constexpr int mg_isolatedPawnPenalty=-15;
constexpr int eg_isolatedPawnPenalty=-8;
constexpr int mg_doubledPawnPenalty=-12;
constexpr int eg_doubledPawnPenalty=-15;
constexpr int mg_connectedPawnsBonus=10;
constexpr int eg_connectedPawnsBonus=8;
constexpr int mg_protectedPawnsBonus=20;
constexpr int eg_protectedPawnsBonus=40;
}
constexpr std::array<int,6> middlegame_value = { 82, 337, 365, 477, 1025,  0};
constexpr std::array<int,6> endgame_value = { 94, 281, 297, 512,  936,  0};
std::array<std::array<int,64>,12> middlegame_pesto_score;
std::array<std::array<int,64>,12> endgame_pesto_score;
void initEval(){
    for(int piece=WP;piece<=WK;piece++){
        for(int sq=0;sq<64;sq++){
            middlegame_pesto_score[piece][sq]=middlegame_value[piece]+pesto::middlegame_Tables[piece][sq];
            endgame_pesto_score[piece][sq]=endgame_value[piece]+pesto::endgame_Tables[piece][sq];
            middlegame_pesto_score[piece+6][sq]=middlegame_value[piece]+pesto::middlegame_Tables[piece][sq^56];
            endgame_pesto_score[piece+6][sq]=endgame_value[piece]+pesto::endgame_Tables[piece][sq^56];
        }
    }
}
void evalPawns(Color color,std::array<int,2> &middlegame,std::array<int,2>& endgame,const Board& board){
    int myOff=color*6;
    int oppOff=(color^1)*6;
    Bitboard pawns=board.pieces[WP+myOff];
    while(pawns){
        int sq=popLSB(pawns);
        if(!((color==BLACK?blackPawnPassedMask[sq]:whitePawnPassedMask[sq]) & board.pieces[WP+oppOff])){//passed pawns
            int rank=sq/8;
            int relativeRank=color==BLACK?6-rank:rank-1;
            middlegame[color]+=pesto::mg_passedPawnBonus[relativeRank];
            endgame[color]+=pesto::eg_passedPawnBonus[relativeRank];
        }
        
        if(!(adjacentFileMasks[sq%8] & board.pieces[WP+myOff])){//isolated pawns
            middlegame[color]+=pesto::mg_isolatedPawnPenalty;
            endgame[color]+=pesto::eg_isolatedPawnPenalty;
        }
        //doubled pawns
        int countOfPawnsOnFile=popCount(fileMasks[sq%8]&board.pieces[WP+myOff]);
        middlegame[color]+=(countOfPawnsOnFile-1)*pesto::mg_doubledPawnPenalty;
        endgame[color]+=(countOfPawnsOnFile-1)*pesto::eg_doubledPawnPenalty;

        //connected
        Bitboard mask=pawnAttacks[color^1][sq];
        if((sq%8)>0) setBit(mask, sq-1);
        if((sq%8)<7) setBit(mask, sq+1);
        if(mask & board.pieces[WP+myOff]){
            middlegame[color]+=pesto::mg_connectedPawnsBonus;
            endgame[color]+=pesto::eg_connectedPawnsBonus;
        }

        int protectedBonus_mg=0,protectedBonus_eg=0;
        mask=pawnAttacks[color^1][sq];
        if(mask & board.pieces[WP+myOff]){
            protectedBonus_mg=pesto::mg_protectedPawnsBonus;
            protectedBonus_eg=pesto::eg_protectedPawnsBonus;
        }
        middlegame[color]+=protectedBonus_mg;
        endgame[color]+=protectedBonus_eg;

    }
}
int eval(const Board& board){
    std::array<int,2>middlegame={0,0};
    std::array<int,2>endgame={0,0};
    int bishopPair[2]={0,0};
    int gamephase=0;
    for(int piece=WP;piece<=BK;piece++){
        Bitboard pieces=board.pieces[piece];
        while(pieces){
            int sq=popLSB(pieces);
            Color color=pieceColor((Piece) piece);
            middlegame[color] += middlegame_pesto_score[piece][sq];
            endgame[color]+=endgame_pesto_score[piece][sq];
            if(piece%6==WB) bishopPair[color]++;
            gamephase+=gamephaseInc[piece];
        }
    }
    //add bishop pair bonus if exists
    if(bishopPair[WHITE]>=2){
        middlegame[WHITE]+=pesto::bishopPairBonusMiddlegame;
        endgame[WHITE]+=pesto::bishopPairBonusEndgame;
    }
    if(bishopPair[BLACK]>=2){
        middlegame[BLACK]+=pesto::bishopPairBonusMiddlegame;
        endgame[BLACK]+=pesto::bishopPairBonusEndgame;
    }
    
    //pawn bonuses
    evalPawns(WHITE, middlegame, endgame, board);
    evalPawns(BLACK, middlegame, endgame, board);


    //taper from mid to end
    int middleGameScore=middlegame[board.sideToMove]-middlegame[board.sideToMove^1];
    int endGameScore=endgame[board.sideToMove]-endgame[board.sideToMove^1];
    int middleGamePhase=gamephase;
    middleGamePhase=std::min(middleGamePhase,24);
    int endGamePhase=24-middleGamePhase;

    return (middleGamePhase*middleGameScore + endGamePhase*endGameScore)/24;
}