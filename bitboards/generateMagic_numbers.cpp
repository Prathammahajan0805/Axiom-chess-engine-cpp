#include "bitboards.h"
#include<iostream>
#include <iomanip>
#include <vector>
Bitboard bishopAttacksMask[64];
Bitboard rookAttacksMask[64];
Bitboard rookMagic[64];
Bitboard bishopMagic[64];
uint64_t state = 1804289383ULL;

uint64_t random64() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
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

Bitboard findMagic(int sq,bool isBishop){
    Bitboard mask= isBishop? bishopAttacksMask[sq]:rookAttacksMask[sq];
    int bits=popCount(mask);
    std::vector<Bitboard> occupancy(1<<bits);
    std::vector<Bitboard> attacks(1<<bits);
    for(int i=0;i<1<<bits;i++){
        occupancy[i]=setOccupancy(i,bits, mask);
        attacks[i] = isBishop ? bishopAttacksOnTheFly(sq, occupancy[i]):rookAttacksOnTheFly(sq, occupancy[i]);
    }
    
    for (int randomCount = 0; randomCount < 100000000;randomCount++){
        Bitboard magic=random64() & random64() & random64();
        if (popCount((mask * magic) & 0xFF00000000000000ULL) < 6)
            continue;
        std::vector<Bitboard> attackTable(1<<bits,0ULL);
        bool fail=false;
        for(int i=0;i<1<<bits;i++){
            Bitboard index=(occupancy[i]*magic)>>(64-bits);
            
            if(attackTable[index]==0ULL){
                attackTable[index]=attacks[i];
            }else if(attackTable[index]!=attacks[i]){
                fail=true;
                break;
            }
        }
        if(!fail){
            return magic;
        }
    }
    return 0ULL;
}

void generateMagicArrayConstants(){
    for(int sq=0;sq<64;sq++){
        rookMagic[sq]=findMagic(sq,false);
        bishopMagic[sq]=findMagic(sq,true);
    }
}

void printMagicArrays(){
    std::cout << "const uint64_t rookMagicNumbers[64] = {\n";

    for (int sq = 0; sq < 64; sq++) {
        std::cout << "0x"
            << std::hex << std::setw(16) << std::setfill('0')
            << rookMagic[sq]
            << "ULL";

        if (sq != 63)
            std::cout << ",";

        std::cout << "\n";
    }

    std::cout << "};\n";
    std::cout<<"\n";
    std::cout << "const uint64_t bishopMagicNumbers[64] = {\n";

    for (int sq = 0; sq < 64; sq++) {
        std::cout << "0x"
            << std::hex << std::setw(16) << std::setfill('0')
            << bishopMagic[sq]
            << "ULL";

        if (sq != 63)
            std::cout << ",";

        std::cout << "\n";
    }

    std::cout << "};\n";
}

int main(){
    initBishopReleventOccupancyMask();
    initRookReleventOccupancyMask();
    generateMagicArrayConstants();
    printMagicArrays();
    return 0;
}
