#include "board.h"
#include"evalualtion.h"
#include "types.h"
#include<array>
std::array<const int,12> gamephaseInc={0,01,1,1,1,1,2,2,4,4,0,0};
std::array<const int*,6> middlegame_Tables={
    middlegamePawn_Table,
    middlegameKnight_table,
    middlegameBishop_table,
    middlegameRook_table,
    middlegameQueen_table,
    middlegameKing_table
};
std::array<const int*,6> endgame_Tables={
    endgamePawn_Table,
    endgameKnight_table,
    endgameBishop_table,
    endgameRook_table,
    endgameQueen_table,
    endgameKing_table
};
std::array<int,6> middlegame_value = { 82, 337, 365, 477, 1025,  0};
std::array<int,6> endgame_value = { 94, 281, 297, 512,  936,  0};
std::array<std::array<int,64>,12> middlegame_pesto_score;
std::array<std::array<int,64>,12> endgame_pesto_score;
void initEval(){
    for(int piece=WP;piece<=WK;piece++){
        for(int sq=0;sq<64;sq++){
            middlegame_pesto_score[piece][sq]=middlegame_value[piece]+middlegame_Tables[piece][sq];
            endgame_pesto_score[piece][sq]=endgame_value[piece]+endgame_Tables[piece][sq];
            middlegame_pesto_score[piece+6][sq]=middlegame_value[piece]+middlegame_Tables[piece][sq^56];
            endgame_pesto_score[piece+6][sq]=endgame_value[piece]+endgame_Tables[piece][sq^56];
        }
    }
}

int eval(const Board& board){
    int middlegame[2]={0,0};
    int endgame[2]={0,0};
    int gamephase=0;
    for(int sq=0;sq<64;sq++){
        Piece piece=board.pieceOn[sq];
        if(piece==NO_PIECE) continue;
        Color color=piece>=6?BLACK:WHITE;
        middlegame[color] += middlegame_pesto_score[piece][sq];
        endgame[color]+=endgame_pesto_score[piece][sq];
        gamephase+=gamephaseInc[piece];
    }

    //taper from mid to end
    int middleGameScore=middlegame[board.sideToMove]-middlegame[board.sideToMove^1];
    int endGameScore=endgame[board.sideToMove]-endGameScore[board.sideToMove^1];
    int middleGamePhase=gamephase;
    if(middleGamePhase>24) middleGamePhase=24;
    int endGamePhase=24-middleGamePhase;

    return (middleGamePhase*middleGameScore + endGamePhase*endGameScore)/24;
}