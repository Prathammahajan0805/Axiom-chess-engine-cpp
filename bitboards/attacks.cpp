#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "magic.h"
#include <cstdint>

Bitboard pawnAttacks[2][64];
Bitboard knightAttacks[64];
Bitboard kingAttacks[64];
Bitboard bishopAttacksMask[64];
Bitboard rookAttacksMask[64];
Bitboard rookAttacks[64][4096];
Bitboard bishopAttacks[64][512];
Bitboard line[64][64];
Bitboard between[64][64];
int rookShifts[64];
int bishopShifts[64];

void initShifts(){
    for(int i=0;i<64;i++){
        rookShifts[i]=popCount(rookAttacksMask[i]);
        bishopShifts[i]=popCount(bishopAttacksMask[i]);
    }
}
static Bitboard maskKnightAttacks(int sq){
    Bitboard attacks=0ULL;
    int rank=sq/8;
    int file=sq%8;
    constexpr int dr[8] = {2,2,1,-1,-2,-2,1,-1};
    constexpr int df[8] = {1,-1,2,2,1,-1,-2,-2};
    for(int i=0;i<8;i++){
        int r=rank+dr[i];
        int f=file+df[i];
        if(r>=0 && r< 8 && f>=0 && f<8){
            attacks |= 1ULL << getSquare(f, r);
        }
    }
    return attacks;
}

static Bitboard maskPawnAttacks(Color col,int sq){
    int rank=sq/8;
    int file=sq%8;
    Bitboard attacks = 0ULL;
    int dir = (col == WHITE) ? 1 : -1;

    constexpr int df[2] = {-1, 1};

    for (int i = 0; i < 2; i++) {
        int r = rank + dir;
        int f = file + df[i];

        if(r>=0 && r<8 && f>=0 && f<8){
            attacks |= 1ULL << getSquare(f, r);
        }
    }
    return attacks;
}

static Bitboard maskKingAttacks(int sq){
    int rank=sq/8;
    int file=sq%8;

    Bitboard attacks =0ULL;

    constexpr int dr[8]={1,1,1,0,0,-1,-1,-1};
    constexpr int df[8]={-1,0,1,-1,1,-1,0,1};

    for(int i=0;i<8;i++){
        int r=rank+dr[i];
        int f=file+df[i];

        if(r>=0 && r<8 && f>=0 && f<8){
            attacks |= 1ULL<< getSquare(f,r);
        }
    }
    return attacks;
}

void initLeaperAttacks(){
    for( int sq = A1;sq<= H8 ;sq++){
        pawnAttacks[WHITE][sq]=maskPawnAttacks(WHITE,sq);
        pawnAttacks[BLACK][sq]=maskPawnAttacks(BLACK,  sq);

        knightAttacks[sq]=maskKnightAttacks(sq);

        kingAttacks[sq]=maskKingAttacks(sq);
    }
}

void initBishopReleventOccupancyMask(){
    for(int sq=0;sq<64;sq++){
        int rank=sq/8;
        int file=sq%8;
        Bitboard mask=0ULL;
        for(int r=rank+1,f=file+1;r<7 && f<7;r++,f++){
            setBit(mask,getSquare(f, r));
        }

        for(int r=rank+1,f=file-1;r<7 && f>0;r++,f--){
            setBit(mask,getSquare(f, r));
        }

        for(int r=rank-1,f=file+1;r>0 && f<7;r--,f++){
            setBit(mask,getSquare(f, r));
        }

        for(int r=rank-1,f=file-1;r>0 && f>0;r--,f--){
            setBit(mask,getSquare(f, r));
        }

        bishopAttacksMask[sq]=mask;
    }
}

void initRookReleventOccupancyMask(){
    for(int sq=0;sq<64;sq++){
        int rank=sq/8;
        int file=sq%8;

        Bitboard mask=0ULL; 

        for(int r=rank+1;r<7;r++){
            setBit(mask,getSquare(file,r));
        }

        for(int r=rank-1;r>0;r--){
            setBit(mask,getSquare(file,r));
        }

        for(int f=file+1;f<7;f++){
            setBit(mask,getSquare(f, rank));
        }

        for(int f=file-1;f>0;f--){
            setBit(mask,getSquare(f, rank));
        }

        rookAttacksMask[sq]=mask;
    }    
}

Bitboard setOccupancy(int index,int bitsInMask,Bitboard mask){
    Bitboard occupancy=0ULL;
    for(int i=0;i<bitsInMask;i++){
        int square=popLSB(mask);
        if(index & (1ULL<<i)){
            setBit(occupancy, square);
        }
    }
    return occupancy;
}

Bitboard bishopAttacksOnTheFly(int sq,Bitboard occupancy){
    int rank=sq/8;
    int file=sq%8;
    Bitboard mask=0ULL;
    for(int r=rank+1,f=file+1;r<8 && f<8;r++,f++){
        setBit(mask,getSquare(f, r));
        if(getBit(occupancy,getSquare(f, r))) break;
    }

    for(int r=rank+1,f=file-1;r<8 && f>=0;r++,f--){
        setBit(mask,getSquare(f, r));
        if(getBit(occupancy,getSquare(f, r))) break;
    }

    for(int r=rank-1,f=file+1;r>=0 && f<8;r--,f++){
        setBit(mask,getSquare(f, r));
        if(getBit(occupancy,getSquare(f, r))) break;
    }

    for(int r=rank-1,f=file-1;r>=0 && f>=0;r--,f--){
        setBit(mask,getSquare(f, r));
        if(getBit(occupancy,getSquare(f, r))) break;
    }
    return mask;
}

Bitboard rookAttacksOnTheFly(int sq,Bitboard occupancy){
    int rank=sq/8;
    int file=sq%8;

    Bitboard mask=0ULL; 

    for(int r=rank+1;r<8;r++){
        setBit(mask,getSquare(file,r));
        if(getBit(occupancy,getSquare(file, r))) break;
    }

    for(int r=rank-1;r>=0;r--){
        setBit(mask,getSquare(file,r));
        if(getBit(occupancy,getSquare(file, r))) break;
    }

    for(int f=file+1;f<8;f++){
        setBit(mask,getSquare(f, rank));
        if(getBit(occupancy,getSquare(f,rank))) break;
    }

    for(int f=file-1;f>=0;f--){
        setBit(mask,getSquare(f, rank));
        if(getBit(occupancy,getSquare(f,rank))) break;
    }
    return mask;
}

void initSliderAttacks(bool isBishop){
    for(int sq=0;sq<64;sq++){
        Bitboard mask=isBishop?bishopAttacksMask[sq]:rookAttacksMask[sq];
        int bitInMask=popCount(mask);
        int maxOccupancy=1 << bitInMask;

        for(int index=0;index<maxOccupancy;index++){
            Bitboard occupancy=setOccupancy(index, bitInMask, mask);
            Bitboard magicInd=magicIndex(occupancy,isBishop?bishopMagicNumbers[sq]:rookMagicNumbers[sq],64-bitInMask, mask);
            if(isBishop){
                bishopAttacks[sq][magicInd]=bishopAttacksOnTheFly(sq, occupancy);
            }else{
                rookAttacks[sq][magicInd]=rookAttacksOnTheFly(sq, occupancy);
            }
        }
    }
}
void initAllSliderAttacks(){
    initBishopReleventOccupancyMask();
    initRookReleventOccupancyMask();
    initSliderAttacks(true);
    initSliderAttacks(false);
}
int getStep(int from,int to){
    int dr = (to / 8) - (from / 8);
    int df = (to % 8) - (from % 8);
    dr = (dr > 0) - (dr < 0);
    df = (df > 0) - (df < 0);

    return dr * 8 + df;
}
void initLineBetween(){
    memset(line, 0, sizeof(line));
    memset(between, 0, sizeof(between));
    for(int from=0;from<64;from++){
        Bitboard queenRay = rookAttacksOnTheFly(from, 0ULL) | bishopAttacksOnTheFly(from, 0ULL);
        for(int to=0;to<64;to++){
            if (from == to) {
                setBit(line[from][from], from);
                continue;
            }
            if(!getBit(queenRay,to)) continue;
            int step=getStep(from, to);
            int sq = from;
            while (sq != to) {
                setBit(line[from][to], sq);
                sq += step;
                if (sq != to)
                    setBit(between[from][to], sq);
            }

            setBit(line[from][to], to);
        }
    }
}

