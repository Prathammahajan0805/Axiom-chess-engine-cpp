#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include"evaluation.h"
#include "types.h"
#include"search.h"
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
constexpr int mg_pawnShieldBonus = 10;
constexpr int mg_openFileNearKingPenalty = -25;
constexpr int mg_semiOpenFileNearKingPenalty = -12;

constexpr int attackWeight[6] = {0, 20, 20, 40, 80, 0};
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

constexpr int tempoBonus_mg = 15;
constexpr int tempoBonus_eg = 5;

// Rook file/rank bonuses
constexpr int mg_rookOpenFile = 25;
constexpr int mg_rookSemiOpenFile = 12;
constexpr int mg_rookOnSeventh = 15;
constexpr int eg_rookOnSeventh = 25;

// Passed pawn king distance (per square of distance)
constexpr int eg_passedPawnOwnKingDist = -4;   // penalty per square own king is far
constexpr int eg_passedPawnEnemyKingDist = 6;  // bonus per square enemy king is far

// Threats
constexpr int mg_hangingPawnAttack = -18;
constexpr int eg_hangingPawnAttack = -12;
constexpr int mg_threatByMinor[6] = {0, 0, 0, 30, 45, 55};  // indexed by victim piece type
constexpr int eg_threatByMinor[6] = {0, 0, 0, 35, 50, 60};
constexpr int mg_threatByRook[6]  = {0, 0, 0, 0, 35, 45};
constexpr int eg_threatByRook[6]  = {0, 0, 0, 0, 40, 50};

// Outposts
constexpr int mg_knightOutpost = 25;
constexpr int eg_knightOutpost = 15;
constexpr int mg_bishopOutpost = 15;
constexpr int eg_bishopOutpost = 10;

// Backward pawns
constexpr int mg_backwardPawnPenalty = -10;
constexpr int eg_backwardPawnPenalty = -6;

// King safety: pawn storm
constexpr int mg_pawnStormPenalty[8] = {0, 0, -2, -5, -10, -18, -28, -40}; // indexed by rank (relative to attacker)

// Virtual queen mobility (king danger)
constexpr int mg_virtualMobilityWeight = 3;

// Bad bishop
constexpr int mg_badBishopPenalty = -3; // per own pawn on bishop's color
constexpr int eg_badBishopPenalty = -5;

// Space
constexpr int mg_spaceBonus = 2; // per controlled square in enemy half

// Material imbalance
constexpr int mg_knightClosedBonus = 4;  // per own pawn beyond 4, closed position
constexpr int mg_bishopOpenBonus = 4;
}
Bitboard pawnAttacksBB(Color color, const Board& board){
    int off = color*6;
    Bitboard pawns = board.pieces[WP+off];

    if(color == WHITE){
        Bitboard left  = (pawns & NOT_FILE_A) << 7;
        Bitboard right = (pawns & NOT_FILE_H) << 9;
        return left | right;
    } else {
        Bitboard left  = (pawns & NOT_FILE_A) >> 9;
        Bitboard right = (pawns & NOT_FILE_H) >> 7;
        return left | right;
    }
}
inline int chebyshevDist(int sq1, int sq2){
    int f1 = sq1%8, r1 = sq1/8, f2 = sq2%8, r2 = sq2/8;
    return std::max(std::abs(f1-f2), std::abs(r1-r2));
}
PawnEntry* PawnHashTable::probe(uint64_t pawnHash){
    PawnEntry &e = table[pawnHash & (PAWN_HASH_SIZE-1)];
    if(e.key == pawnHash) return &e;
    return nullptr;
}
void evalKingShield(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6;
    int kingSq = bitScan(board.pieces[WK+myOff]);
    int kf = kingSq % 8, kr = kingSq / 8;

    for(int f = std::max(0, kf-1); f <= std::min(7, kf+1); f++){
        Bitboard fileMask = fileMasks[f] & board.pieces[WP+myOff];
        if(!fileMask) continue;
        int pawnRank;
        if(color == WHITE){
            Bitboard ahead = fileMask & ~((1ULL << ((kr+1)*8)) - 1);
            pawnRank = ahead ? (bitScan(ahead)/8) : -1;
        } else {
            Bitboard ahead = fileMask & ((1ULL << (kr*8)) - 1);
            Bitboard reversed = __builtin_bswap64(ahead);
            pawnRank = ahead ? (63 - bitScan(reversed))/8 : -1;
        }
        if(pawnRank != -1){
            int dist = std::abs(pawnRank - kr);
            if(dist <= 2) middlegame[color] += pesto::mg_pawnShieldBonus;
        }
    }
}
void evalKingAttackers(Color color, std::array<int,2>& middlegame, const Board& board){
    int oppOff = (color^1)*6; // attacker is the opposite color
    int myOff = color*6;
    int kingSq = bitScan(board.pieces[WK+myOff]);
    Bitboard zone = kingAttacks[kingSq];
    setBit(zone, kingSq);

    int attackUnits = 0;
    bool oppHasQueen = board.pieces[WQ+oppOff]!=0;
    for(int pieceType = WN; pieceType <= WQ; pieceType++){
        Bitboard pieces = board.pieces[pieceType + oppOff];
        while(pieces){
            int sq = popLSB(pieces);
            Bitboard attacks;
            switch(pieceType){
                case WN: attacks = knightAttacks[sq]; break;
                case WB: attacks = getBishopAttacks(sq, board.occupied); break;
                case WR: attacks = getRookAttacks(sq, board.occupied); break;
                case WQ: attacks = getBishopAttacks(sq, board.occupied) | getRookAttacks(sq, board.occupied); break;
                default: attacks = 0;
            }
            if(attacks & zone){
                int overlap = popCount(attacks & zone);
                attackUnits += pesto::attackWeight[pieceType] * overlap / 2.5;
            }
        }
    }
    if(!oppHasQueen) attackUnits = attackUnits / 2;
    attackUnits = std::min(attackUnits, 99);
    middlegame[color] -= kingDangerTable[attackUnits];
}
void PawnHashTable::store(uint64_t pawnHash, const std::array<int,2>& mg, const std::array<int,2>& eg){
    PawnEntry &e = table[pawnHash & (PAWN_HASH_SIZE-1)];
    e.key = pawnHash;
    e.mg[0]=mg[0]; e.mg[1]=mg[1];
    e.eg[0]=eg[0]; e.eg[1]=eg[1];
}
void evalKingFileSafety(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    int kingSq = bitScan(board.pieces[WK+myOff]);
    int kf = kingSq % 8;

    for(int f = std::max(0, kf-1); f <= std::min(7, kf+1); f++){
        bool myPawns = fileMasks[f] & board.pieces[WP+myOff];
        bool oppPawns = fileMasks[f] & board.pieces[WP+oppOff];
        if(!myPawns && !oppPawns)
            middlegame[color] += pesto::mg_openFileNearKingPenalty;
        else if(!myPawns)
            middlegame[color] += pesto::mg_semiOpenFileNearKingPenalty;
    }
}

constexpr std::array<int,6> middlegame_value = { 82, 337, 365, 477, 1025,  0};
constexpr std::array<int,6> endgame_value = { 94, 281, 297, 512,  936,  0};
std::array<std::array<int,64>,12> middlegame_pesto_score;
std::array<std::array<int,64>,12> endgame_pesto_score;
void initEval(){
    for(int piece=WP;piece<=WK;piece++){
        for(int sq=0;sq<64;sq++){
            middlegame_pesto_score[piece][sq]=middlegame_value[piece]+pesto::middlegame_Tables[piece][sq^56];
            endgame_pesto_score[piece][sq]=endgame_value[piece]+pesto::endgame_Tables[piece][sq^56];
            middlegame_pesto_score[piece+6][sq]=middlegame_value[piece]+pesto::middlegame_Tables[piece][sq];
            endgame_pesto_score[piece+6][sq]=endgame_value[piece]+pesto::endgame_Tables[piece][sq];
        }
    }
}
void evalThreats(Color color, std::array<int,2>& middlegame, std::array<int,2>& endgame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    Bitboard myPawnAttacks = pawnAttacksBB(color, board);

    // pawn attacks on enemy pieces
    for(int pieceType = WN; pieceType <= WQ; pieceType++){
        Bitboard targets = board.pieces[pieceType+oppOff] & myPawnAttacks;
        if(targets){
            int count = popCount(targets);
            middlegame[color] += count * pesto::mg_hangingPawnAttack * -1; // bonus for attacker
            endgame[color]    += count * pesto::eg_hangingPawnAttack * -1;
        }
    }

    // minor piece threats on rooks/queens
    Bitboard knights = board.pieces[WN+myOff];
    Bitboard bishops = board.pieces[WB+myOff];
    Bitboard minorAttacks = 0;
    while(knights){ int sq=popLSB(knights); minorAttacks |= knightAttacks[sq]; }
    while(bishops){ int sq=popLSB(bishops); minorAttacks |= getBishopAttacks(sq, board.occupied); }

    for(int pieceType = WR; pieceType <= WQ; pieceType++){
        Bitboard targets = board.pieces[pieceType+oppOff] & minorAttacks;
        if(targets){
            int count = popCount(targets);
            middlegame[color] += count * pesto::mg_threatByMinor[pieceType];
            endgame[color]    += count * pesto::eg_threatByMinor[pieceType];
        }
    }

    // rook threats on queen
    Bitboard rooks = board.pieces[WR+myOff];
    Bitboard rookAttacks = 0;
    while(rooks){ int sq=popLSB(rooks); rookAttacks |= getRookAttacks(sq, board.occupied); }
    Bitboard queenTarget = board.pieces[WQ+oppOff] & rookAttacks;
    if(queenTarget){
        int count = popCount(queenTarget);
        middlegame[color] += count * pesto::mg_threatByRook[WQ];
        endgame[color]    += count * pesto::eg_threatByRook[WQ];
    }
}
void evalKingStorm(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    int kingSq = bitScan(board.pieces[WK+myOff]);
    int kf = kingSq%8;

    for(int f = std::max(0,kf-1); f <= std::min(7,kf+1); f++){
        Bitboard enemyPawnsOnFile = fileMasks[f] & board.pieces[WP+oppOff];
        while(enemyPawnsOnFile){
            int sq = popLSB(enemyPawnsOnFile);
            int rank = sq/8;
            int relativeRank = color==WHITE ? 7-rank : rank; // how far advanced toward our king
            middlegame[color] += pesto::mg_pawnStormPenalty[relativeRank];
        }
    }
}

void evalVirtualMobility(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6;
    int kingSq = bitScan(board.pieces[WK+myOff]);
    Bitboard virtualAttacks = getBishopAttacks(kingSq, board.occupied) | getRookAttacks(kingSq, board.occupied);
    int mobility = popCount(virtualAttacks);
    middlegame[color] -= mobility * pesto::mg_virtualMobilityWeight;
}
void evalBadBishop(Color color, std::array<int,2>& middlegame, std::array<int,2>& endgame, const Board& board){
    int myOff = color*6;
    Bitboard bishops = board.pieces[WB+myOff];
    Bitboard lightSquares = 0x55AA55AA55AA55AAULL;
    Bitboard darkSquares  = ~lightSquares;

    while(bishops){
        int sq = popLSB(bishops);
        bool onLight = (lightSquares >> sq) & 1ULL;
        Bitboard sameColorPawns = board.pieces[WP+myOff] & (onLight ? lightSquares : darkSquares);
        int count = popCount(sameColorPawns);
        middlegame[color] += count * pesto::mg_badBishopPenalty;
        endgame[color]    += count * pesto::eg_badBishopPenalty;
    }
}
void evalSpace(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6;
    Bitboard enemyPawnAttacks = pawnAttacksBB((Color)(color^1), board);

    // enemy half of the board
    Bitboard enemyHalf = color==WHITE ? 0xFFFFFFFF00000000ULL : 0x00000000FFFFFFFFULL;

    Bitboard ownPieces = 0;
    for(int p = WN; p <= WQ; p++) ownPieces |= board.pieces[p+myOff];

    Bitboard controlled = 0;
    Bitboard t = ownPieces;
    while(t){
        int sq = popLSB(t);
        int pieceType = -1;
        for(int p = WN; p <= WQ; p++){
            if(getBit(board.pieces[p+myOff], sq)){ pieceType = p; break; }
        }
        Bitboard attacks;
        switch(pieceType){
            case WN: attacks = knightAttacks[sq]; break;
            case WB: attacks = getBishopAttacks(sq, board.occupied); break;
            case WR: attacks = getRookAttacks(sq, board.occupied); break;
            case WQ: attacks = getBishopAttacks(sq, board.occupied) | getRookAttacks(sq, board.occupied); break;
            default: attacks = 0;
        }
        controlled |= attacks;
    }

    Bitboard safeSpace = controlled & enemyHalf & ~enemyPawnAttacks;
    middlegame[color] += popCount(safeSpace) * pesto::mg_spaceBonus;
}
void evalMaterialImbalance(Color color, std::array<int,2>& middlegame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    int myPawns = popCount(board.pieces[WP+myOff]);
    int totalPawns = myPawns + popCount(board.pieces[WP+oppOff]);

    int myKnights = popCount(board.pieces[WN+myOff]);
    int myBishops = popCount(board.pieces[WB+myOff]);

    // knights like closed (many pawns) positions
    if(totalPawns >= 12){
        middlegame[color] += myKnights * pesto::mg_knightClosedBonus;
    }

    // bishops like open (few pawns) positions
    if(totalPawns <= 8){
        middlegame[color] += myBishops * pesto::mg_bishopOpenBonus;
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

            int file = sq%8;
            int promoSq = (color==WHITE) ? (56+file) : file;
            int myKingSq = bitScan(board.pieces[WK+myOff]);
            int oppKingSq = bitScan(board.pieces[WK+oppOff]);
            endgame[color] += chebyshevDist(myKingSq, promoSq) * pesto::eg_passedPawnOwnKingDist;
            endgame[color] += chebyshevDist(oppKingSq, promoSq) * pesto::eg_passedPawnEnemyKingDist;
        }
        
        if(!(adjacentFileMasks[sq%8] & board.pieces[WP+myOff])){//isolated pawns
            middlegame[color]+=pesto::mg_isolatedPawnPenalty;
            endgame[color]+=pesto::eg_isolatedPawnPenalty;
        }
        //doubled pawns
        int countOfPawnsOnFile=popCount(fileMasks[sq%8]&board.pieces[WP+myOff]);
        middlegame[color]+=(countOfPawnsOnFile-1)*pesto::mg_doubledPawnPenalty;
        endgame[color]+=(countOfPawnsOnFile-1)*pesto::eg_doubledPawnPenalty;
        int file = sq%8;
        int rank = sq/8;
        bool canBeDefended = false;
        Bitboard adjFiles = adjacentFileMasks[file] & board.pieces[WP+myOff];
        Bitboard t = adjFiles;
        while(t){
            int otherSq = popLSB(t);
            int otherRank = otherSq/8;
            if(color==WHITE ? (otherRank < rank) : (otherRank > rank)){
                canBeDefended = true;
                break;
            }
        }
        if(!canBeDefended){
            int aheadSq = color==WHITE ? sq+8 : sq-8;
            if(aheadSq >= 0 && aheadSq < 64){
                bool aheadControlledByEnemy = pawnAttacks[color][aheadSq] & board.pieces[WP+oppOff];
                if(aheadControlledByEnemy){
                    middlegame[color] += pesto::mg_backwardPawnPenalty;
                    endgame[color]    += pesto::eg_backwardPawnPenalty;
                }
            }
        }
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
void evalPawnsCached(const Board& board, PawnHashTable& pht,
                      std::array<int,2>& middlegame, std::array<int,2>& endgame){
    PawnEntry* hit = pht.probe(board.pawnHash);
    if(hit){
        middlegame[WHITE]+=hit->mg[WHITE]; middlegame[BLACK]+=hit->mg[BLACK];
        endgame[WHITE]+=hit->eg[WHITE];    endgame[BLACK]+=hit->eg[BLACK];
        return;
    }

    std::array<int,2> mg[2]={0,0}, eg[2]={0,0};
    std::array<int,2> mgArr{0,0}, egArr{0,0};
    evalPawns(WHITE, mgArr, egArr, board);
    evalPawns(BLACK, mgArr, egArr, board);

    pht.store(board.pawnHash, mgArr, egArr);

    middlegame[WHITE]+=mgArr[WHITE]; middlegame[BLACK]+=mgArr[BLACK];
    endgame[WHITE]+=egArr[WHITE];    endgame[BLACK]+=egArr[BLACK];
}

void evalMobility(Color color, std::array<int,2>& middlegame, std::array<int,2>& endgame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    Bitboard ownPieces = 0;
    for(int p = WP; p <= WK; p++) ownPieces |= board.pieces[p+myOff];

    Bitboard enemyPawnAttacks = pawnAttacksBB((Color)(color^1), board);
    Bitboard mobilityMask = ~ownPieces & ~enemyPawnAttacks;

    for(int pieceType = WN; pieceType <= WQ; pieceType++){
        Bitboard pieces = board.pieces[pieceType + myOff];
        while(pieces){
            int sq = popLSB(pieces);
            Bitboard attacks;
            switch(pieceType){
                case WN: attacks = knightAttacks[sq]; break;
                case WB: attacks = getBishopAttacks(sq, board.occupied); break;
                case WR: attacks = getRookAttacks(sq, board.occupied); break;
                case WQ: attacks = getBishopAttacks(sq, board.occupied) | getRookAttacks(sq, board.occupied); break;
                default: attacks = 0;
            }
            int count = popCount(attacks & mobilityMask);
            middlegame[color] += mg_mobility[pieceType][count];
            endgame[color]    += eg_mobility[pieceType][count];
        }
    }
}
void evalRooks(Color color, std::array<int,2>& middlegame, std::array<int,2>& endgame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;
    Bitboard rooks = board.pieces[WR+myOff];
    int enemyKingSq = bitScan(board.pieces[WK+oppOff]);
    int enemyKingRank = enemyKingSq/8;

    while(rooks){
        int sq = popLSB(rooks);
        int file = sq%8, rank = sq/8;
        bool myPawns = fileMasks[file] & board.pieces[WP+myOff];
        bool oppPawns = fileMasks[file] & board.pieces[WP+oppOff];

        if(!myPawns && !oppPawns)
            middlegame[color] += pesto::mg_rookOpenFile;
        else if(!myPawns)
            middlegame[color] += pesto::mg_rookSemiOpenFile;

        int relativeRank = color==WHITE ? rank : 7-rank;
        int enemyRelativeKingRank = color==WHITE ? enemyKingRank : 7-enemyKingRank;
        if(relativeRank == 6 && (enemyRelativeKingRank == 7 || (color==WHITE ? (fileMasks[file] & board.pieces[WP+oppOff] & rankMasks[6]) : (fileMasks[file] & board.pieces[WP+oppOff] & rankMasks[1])))){
            middlegame[color] += pesto::mg_rookOnSeventh;
            endgame[color]    += pesto::eg_rookOnSeventh;
        }
    }
}
bool isOutpost(int sq,Color color,const Board& board){
    int rank = sq/8;
    int relativeRank = color==WHITE ? rank : 7-rank;
    if(relativeRank < 3 || relativeRank > 6) return false;//not in enemy territory
    int myOff=color*6;
    int oppOff=(color^1)*6;
    // defended by own pawn
    bool defended = pawnAttacks[color^1][sq] & board.pieces[WP+myOff];
    if(!defended) return false;

    Bitboard aheadMask = color==WHITE ? whitePawnPassedMask[sq] : blackPawnPassedMask[sq];
    Bitboard enemyPawnsAdjacent = aheadMask & adjacentFileMasks[sq%8] & board.pieces[WP+oppOff];
    return !enemyPawnsAdjacent;
}
void evalOutposts(Color color, std::array<int,2>& middlegame, std::array<int,2>& endgame, const Board& board){
    int myOff = color*6, oppOff = (color^1)*6;

    Bitboard knights = board.pieces[WN+myOff];
    while(knights){
        int sq = popLSB(knights);
        if(isOutpost(sq,color,board)){
            middlegame[color] += pesto::mg_knightOutpost;
            endgame[color]    += pesto::eg_knightOutpost;
        }
    }

    Bitboard bishops = board.pieces[WB+myOff];
    while(bishops){
        int sq = popLSB(bishops);
        if(isOutpost(sq,color,board)){
            middlegame[color] += pesto::mg_bishopOutpost;
            endgame[color]    += pesto::eg_bishopOutpost;
        }
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
    //tempo bonus
    middlegame[board.sideToMove] += pesto::tempoBonus_mg;
    endgame[board.sideToMove]    += pesto::tempoBonus_eg;
    
    //pawn bonuses
    evalPawnsCached(board,pht,middlegame,endgame);

    //king safety
    evalKingShield(WHITE, middlegame, board);
    evalKingShield(BLACK, middlegame, board);
    evalKingAttackers(WHITE, middlegame, board);
    evalKingAttackers(BLACK, middlegame, board);
    evalKingFileSafety(WHITE, middlegame, board);
    evalKingFileSafety(BLACK, middlegame, board);

    //eval mobility
    evalMobility(WHITE, middlegame, endgame, board);
    evalMobility(BLACK, middlegame, endgame, board);

    //eval rooks
    evalRooks(WHITE,middlegame,endgame,board);
    evalRooks(BLACK,middlegame,endgame,board);
    
    //eval threats
    evalThreats(WHITE, middlegame,endgame,board);
    evalThreats(BLACK, middlegame,endgame,board);

    //eval outposts
    evalOutposts(WHITE,middlegame,endgame,board);
    evalOutposts(BLACK,middlegame,endgame,board);

    evalBadBishop(WHITE, middlegame, endgame, board);
    evalBadBishop(BLACK, middlegame, endgame, board);

    evalSpace(WHITE, middlegame, board);
    evalSpace(BLACK, middlegame, board);

    evalMaterialImbalance(WHITE, middlegame, board);
    evalMaterialImbalance(BLACK, middlegame, board);

    middlegame[board.sideToMove] += pesto::tempoBonus_mg;
    endgame[board.sideToMove]    += pesto::tempoBonus_eg;

    int middleGameScore=middlegame[board.sideToMove]-middlegame[board.sideToMove^1];
    int endGameScore=endgame[board.sideToMove]-endgame[board.sideToMove^1];
    int middleGamePhase=gamephase;
    middleGamePhase=std::min(middleGamePhase,24);
    int endGamePhase=24-middleGamePhase;

    // endgame scaling
    bool oppositeBishops = false;
    {
        Bitboard wb = board.pieces[WB], bb = board.pieces[BB];
        if(popCount(wb)==1 && popCount(bb)==1){
            int wbSq = bitScan(wb), bbSq = bitScan(bb);
            bool wbLight = ((0x55AA55AA55AA55AAULL >> wbSq) & 1ULL);
            bool bbLight = ((0x55AA55AA55AA55AAULL >> bbSq) & 1ULL);
            oppositeBishops = (wbLight != bbLight);
        }
    }
    int totalNonPawnPieces =
        popCount(board.pieces[WN]) + popCount(board.pieces[BN]) +
        popCount(board.pieces[WB]) + popCount(board.pieces[BB]) +
        popCount(board.pieces[WR]) + popCount(board.pieces[BR]) +
        popCount(board.pieces[WQ]) + popCount(board.pieces[BQ]);

    double egScaleFactor = 1.0;
    if(oppositeBishops && totalNonPawnPieces <= 2) egScaleFactor = 0.5;
    else if(oppositeBishops) egScaleFactor = 0.75;

    endGameScore = (int)(endGameScore * egScaleFactor);

    return (middleGamePhase*middleGameScore + endGamePhase*endGameScore)/24;
}