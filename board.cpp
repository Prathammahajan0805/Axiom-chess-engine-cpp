#include "board.h"
#include<vector>
using namespace std;

    bool Board::isopponent(int a,int b){
        if (b == Piece::None)
            return false;
        return (a & Piece::ColorMask)!=(b & Piece::ColorMask);
    }
    void Board::precomputeknight(){
        static constexpr int offsets[]={17,15,-10,6,-6,10,-15,-17};
        for(int sq=0;sq<64;sq++){
            int fromFile = sq % 8;
            int fromRank = sq / 8;

            for (int offset : offsets) {
                int to = sq + offset;

                if (to < 0 || to >= 64)
                    continue;

                int toFile = to % 8;
                int toRank = to / 8;

                int df = abs(toFile - fromFile);
                int dr = abs(toRank - fromRank);

                if (!((df == 1 && dr == 2) || (df == 2 && dr == 1)))
                    continue;

                knightAttacks[sq].push_back(to);
            }
        }
    }
    void Board::precomputeKing(){
        static constexpr int offsets[]={8,-8,9,7,1,-1,-9,-7};
        for(int sq=0;sq<64;sq++){
            int fromFile = sq % 8;
            int fromRank = sq / 8;

            for (int offset : offsets) {
                int to = sq + offset;

                if (to < 0 || to >= 64)
                    continue;

                int toFile = to % 8;
                int toRank = to / 8;

                int df = abs(toFile - fromFile);
                int dr = abs(toRank - fromRank);

                if (df > 1 || dr > 1)
                    continue;

                kingAttacks[sq].push_back(to);
            }
        }
    }
    void Board::precomputeRookRays(){
        for(int sq=0;sq<64;sq++){
            int currfile=sq%8;
            int currrank=sq/8;
            //right(east) horizontal
            for(int i=sq+1;i<currrank*16;i++){
                rookRays[sq][2].push_back(i);
            }
            //left(west) horizontal
            for(int i=sq-1;i>=currrank*8;i--){
                rookRays[sq][3].push_back(i);
            }
            //up(north) vertical
            for(int i=sq+8;i<64;i+=8){
                rookRays[sq][0].push_back(i);
            }
            //down(south) vertical
            for(int i=sq-8;i>=0;i-=8){
                rookRays[sq][0].push_back(i);
            }
        }
    }
    void Board::precomputeBishopRays(){
        for(int sq=0;sq<64;sq++){
            int currfile=sq%8;
            int currrank=sq/8;
            //upper right(north-east) diagonal
            for(int rank=currrank+1,file=currfile+1;rank<8 && file<8 ;rank++,file++){
                int to=rank*8+file;
                bishopRays[sq][0].push_back(to);
            }
            //upper left(north-west) diagonal
            for(int rank=currrank+1,file=currfile-1;rank<8 && file>=0 ;rank++,file--){
                int to=rank*8+file;
                bishopRays[sq][3].push_back(to);
            }
            //lower left(south-west) diagonal
            for(int rank=currrank-1,file=currfile-1;rank>=0 && file>=0 ;rank--,file--){
                int to=rank*8+file;
                bishopRays[sq][2].push_back(to);
            }
            //lower right(south-east) diagonal
            for(int rank=currrank-1,file=currfile+1;rank>=0 && file<8 ;rank--,file++){
                int to=rank*8+file;
                bishopRays[sq][1].push_back(to);
            }
        }
    }
    void Board::generatePawnMoves(int sq,vector<Move>& moves){
        bool iswhite=(Piece::ColorMask & squares[sq])==Piece::White;
        int file=sq%8;
        static constexpr Promotion promos[] = {QUEEN, ROOK, BISHOP, KNIGHT};
        //normally
        if(iswhite && sq<48 ){

            // just a single pawn push
            if(!squares[sq+8]){
                moves.push_back({sq,
                    sq+8,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    false});
            
            //check if we can do double
                if(sq>=8 && sq<16 && !squares[sq+16]){
                    //double pawn move
                    moves.push_back({sq,
                    sq+16,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    true});
                }
            }
            //left file
            if(file>0){
                //left capture
                if(isopponent(squares[sq], squares[sq+7])){
                        moves.push_back({
                        sq,
                        sq+7,
                        squares[sq],
                        squares[sq+7],
                        NONE,
                        false,
                        false,
                        false
                    });
                }
            }
            //right file
            if(file<7){
                //right capture
                if(isopponent(squares[sq], squares[sq+9])){
                        moves.push_back({
                        sq,
                        sq+9,
                        squares[sq],
                        squares[sq+9],
                        NONE,
                        false,
                        false,
                        false
                    });
                }
            }
        }
        else if(iswhite && sq>=48 && sq<56){
            if(!squares[sq+8]){
                for(auto p:promos)
                    moves.push_back({sq,
                        sq+8,
                        squares[sq],
                        Piece::None,
                        p,
                        false,
                        false,
                        false});
            }
            //check opponent capture moves first file
            if(file>0 ){
                //left capture
                if(isopponent(squares[sq], squares[sq+7])){
                    for(auto p:promos)
                        moves.push_back({sq,
                            sq+7,
                            squares[sq],
                            squares[sq+7],
                            p,
                            false,
                            false,
                            false});
                }
            }
            if(file<7){
                //right capture
                if(isopponent(squares[sq], squares[sq+9])){
                    for(auto p:promos)
                        moves.push_back({sq,
                            sq+9,
                            squares[sq],
                            squares[sq+9],
                            p,
                            false,
                            false,
                            false});
                }
                    
            }
            
            
        }
        //normally
        if(!iswhite && sq>=16){

            // just a single pawn push
            if( !squares[sq-8]){
                moves.push_back({sq,
                sq-8,
                squares[sq],
                Piece::None,
                NONE,
                false,
                false,
                false});
            
            //check if we can do double
                if(sq>=48 && sq<56 && !squares[sq-16]){
                    //double pawn move
                    moves.push_back({sq,
                    sq-16,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    true});
                }
            }


            if(file>0){
                //left capture
                if(isopponent(squares[sq], squares[sq-9])){
                        moves.push_back({
                        sq,
                        sq-9,
                        squares[sq],
                        squares[sq-9],
                        NONE,
                        false,
                        false,
                        false
                    });
                }
            }
            if(file<7){
                //right capture
                if(isopponent(squares[sq], squares[sq-7])){
                        moves.push_back({
                        sq,
                        sq-7,
                        squares[sq],
                        squares[sq-7],
                        NONE,
                        false,
                        false,
                        false
                    });
                }
            }
        }
        else if(!iswhite && sq>=8 && sq<=15){
            if(!squares[sq-8])
                for(auto p:promos)
                    moves.push_back({sq,
                        sq-8,
                        squares[sq],
                        Piece::None,
                        p,
                        false,
                        false,
                        false});
            
            if(file>0){
                //left capture
                if(isopponent(squares[sq], squares[sq-9])){
                    for(auto p:promos)
                        moves.push_back({sq,
                            sq-9,
                            squares[sq],
                            squares[sq-9],
                            p,
                            false,
                            false,
                            false});
                }
            }
            if(file<7){
                //right capture
                if(isopponent(squares[sq], squares[sq-7])){
                    for(auto p:promos)
                        moves.push_back({sq,
                            sq-7,
                            squares[sq],
                            squares[sq-7],
                            p,
                            false,
                            false,
                            false});
                }
                    
            }
            
            
        }
        
    }
    void Board::generateKnightMoves(int sq,vector<Move>& moves){
        for (int to : knightAttacks[sq]) {
            if (!squares[to]) {
                // quiet move
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    false
                });
            }
            else if (isopponent(squares[sq], squares[to])) {
                // capture
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    squares[to],
                    NONE,
                    false,
                    false,
                    false
                });
            }
        }
    }
    void Board::generateKingMoves(int sq,vector<Move>& moves){
        for(int to:kingAttacks[sq]){
            if(!squares[to]){
                //quiet move
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    false

                });
            }else if(isopponent(squares[sq],squares[to])){
                //capture move
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    squares[to],
                    NONE,
                    false,
                    false,
                    false

                });
            }
        }
    }
    void Board::generateRays(const vector<int>& rays,int sq,vector<Move>& moves){
        for(const int to:rays){
            if(!squares[to]){
                //quiet moves
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    Piece::None,
                    NONE,
                    false,
                    false,
                    false

                });
            }else if(isopponent(squares[sq], squares[to])){
                moves.push_back({
                    sq,
                    to,
                    squares[sq],
                    squares[to],
                    NONE,
                    false,
                    false,
                    false

                });
                break;                     
            }else break;
        }
    }
    void Board::generateSlidingPieces(int sq,vector<Move>& moves){
        bool rookMoves=((squares[sq] & Piece::TypeMask) ==Piece::Rook) || ((squares[sq] & Piece::TypeMask) ==Piece::Queen);
        bool bishopMoves=((squares[sq] & Piece::TypeMask) ==Piece::Bishop) || ((squares[sq] & Piece::TypeMask) ==Piece::Queen);
        if(rookMoves){
            for(auto &rays:rookRays[sq]){
                generateRays(rays,sq,moves);
            }
        }if(bishopMoves){
            for(auto &rays:bishopRays[sq]){
                generateRays(rays,sq,moves);
            }
        }
    }