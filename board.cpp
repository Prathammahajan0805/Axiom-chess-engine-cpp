#include "board.h"
#include "move.h"
#include "piece.h"
#include<vector>
#include<iostream>
constexpr int WHITE_KINGSIDE  = 1 << 0;
constexpr int WHITE_QUEENSIDE = 1 << 1;
constexpr int BLACK_KINGSIDE  = 1 << 2;
constexpr int BLACK_QUEENSIDE = 1 << 3;
using namespace std;
int charToPiece(char c) {
    switch(tolower(c)) {
        case 'k': return Piece::King;
        case 'r': return Piece::Rook;
        case 'p': return Piece::Pawn;
        case 'q': return Piece::Queen;
        case 'n': return Piece::Knight;
        case 'b': return Piece::Bishop;
        default:  return Piece::None;
    }
}
int opposite(int a){
    if((a & Piece::ColorMask)==Piece::White){
        return Piece::Black;
    }else{
        return Piece::White;
    }
}
char pieceToChar(int piece) {
    int type = piece & Piece::TypeMask;
    switch(type) {
        case Piece::King:   return 'k';
        case Piece::Rook:   return 'r';
        case Piece::Pawn:   return 'p';
        case Piece::Queen:  return 'q';
        case Piece::Knight: return 'n';
        case Piece::Bishop: return 'b';
        default:            return '.';
    }
}
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
    Board::Board() : squares(64, Piece::None){
        precomputeBishopRays();
        precomputeRookRays();
        precomputeKing();
        precomputeknight();
        loadFEN(startFEN);
    }
    int Board::getPiece(int rank, int file) const {
        return squares[rank * 8 + file];
    }
    
    void Board::setPiece(int rank, int file, int piece) {
        squares[rank * 8 + file] = piece;
    }
    
    int Board::getPiece(int index) const {
        return squares[index];
    }
    
    void Board::setPiece(int index, int piece) {
        squares[index] = piece;
    }
    void Board::loadFEN(const string& fen) {
        squares.assign(64, Piece::None);
        
        int rank = 7, file = 0;
        
        for (char c : fen) {
            if (c == '/') {
                file = 0;
                rank--;
            }
            else if (isdigit(c)) {
                file += (c - '0');
            }
            else {
                int color = isupper(c) ? Piece::White : Piece::Black;
                int pieceType = charToPiece(c);
                setPiece(rank, file, color | pieceType);
                file++;
            }
        }
    }
    void Board::display() const {
        cout << "\n  a b c d e f g h\n";
        
        for (int rank = 7; rank >= 0; rank--) {
            cout << rank + 1 << " ";
            
            for (int file = 0; file < 8; file++) {
                int piece = getPiece(rank, file);
                
                if (piece == Piece::None) {
                    cout << ". ";
                } else {
                    char p = pieceToChar(piece);
                    if (piece & Piece::White) {
                        p = toupper(p);
                    }
                    cout << p << " ";
                }
            }
            cout << rank + 1 << "\n";
        }
        
        cout << "  a b c d e f g h\n\n";
    }
    string Board::toFEN() const {
        string fen = "";
        
        for (int rank = 7; rank >= 0; rank--) {
            int emptyCount = 0;
            
            for (int file = 0; file < 8; file++) {
                int piece = getPiece(rank, file);
                
                if (piece == Piece::None) {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) {
                        fen += to_string(emptyCount);
                        emptyCount = 0;
                    }
                    
                    char p = pieceToChar(piece);
                    if (piece & Piece::White) {
                        p = toupper(p);
                    }
                    fen += p;
                }
            }
            
            if (emptyCount > 0) {
                fen += to_string(emptyCount);
            }
            
            if (rank > 0) fen += '/';
        }
        
        return fen;
    }
    vector<Move> Board::generateMoves(){
        vector<Move> moves;
        for(int sq=0;sq<64;sq++){
            int piece = squares[sq];

            if (!piece)
                continue;

            if ((piece & Piece::ColorMask) != sideToMove)
                continue;

            switch (piece & Piece::TypeMask) {
                case Piece::Pawn:
                    generatePawnMoves(sq, moves);
                    break;

                case Piece::Knight:
                    generateKnightMoves(sq, moves);
                    break;

                case Piece::Bishop:
                case Piece::Rook:
                case Piece::Queen:
                    generateSlidingPieces(sq, moves);
                    break;

                case Piece::King:
                    generateKingMoves(sq, moves);
                    break;
            }
        }
        return moves;
    }
    void Board::makeMove(const Move& m){
        moveHistory.push_back(m);
        history.push_back({castlingRights,enpassentSquare});
        bool isblack=(sideToMove & Piece::ColorMask)==Piece::Black;
        if ((m.capturedpiece & Piece::TypeMask) == Piece::Rook) {
            switch (m.to) {
                case 0:   // a1
                    castlingRights &= ~(WHITE_QUEENSIDE);
                    break;

                case 7:   // h1
                    castlingRights &= ~(WHITE_KINGSIDE);
                    break;

                case 56:  // a8
                    castlingRights &= ~(BLACK_QUEENSIDE);
                    break;

                case 63:  // h8
                    castlingRights &= ~(BLACK_KINGSIDE);
                    break;

            }
        }
        if((m.movedpiece & Piece::TypeMask)==Piece::King){
            if(isblack){
                castlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
            }
            else{
                castlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);;
            }
        }else if((m.movedpiece & Piece::TypeMask)==Piece::Rook){
            switch (m.from) {
                case 0:   // a1
                    castlingRights &= ~(WHITE_QUEENSIDE);
                    break;

                case 7:   // h1
                    castlingRights &= ~(WHITE_KINGSIDE);
                    break;

                case 56:  // a8
                    castlingRights &= ~(BLACK_QUEENSIDE);
                    break;

                case 63:  // h8
                    castlingRights &= ~(BLACK_KINGSIDE);
                    break;
            }
        }
        enpassentSquare=-1;
        squares[m.to]=m.movedpiece;
        squares[m.from]=Piece::None;
        if(m.promotion!=NONE){
            squares[m.to]=(sideToMove & Piece::ColorMask) |m.promotion;
        }
        if (m.isEnPassant) {
            if (sideToMove == Piece::White)
                squares[m.to - 8] = Piece::None;
            else
                squares[m.to + 8] = Piece::None;
        }
        if(m.isDoublePawnPush){
            
            if(isblack){
                enpassentSquare=m.to+8;
            }else{
                enpassentSquare=m.to-8;
            }
        }
        if(m.isCastle){
            int toFile=m.to%8;
            if(toFile==6){
                if(isblack){
                    squares[63]=Piece::None;
                    squares[61]=Piece::Black | Piece::Rook;
                }
                else{
                    squares[7]=Piece::None;
                    squares[5]=Piece::White | Piece::Rook;
                }
            }else{
                if(isblack){
                    squares[56]=Piece::None;
                    squares[59]=Piece::Black | Piece::Rook;
                }
                else{
                    squares[0]=Piece::None;
                    squares[3]=Piece::White | Piece::Rook;
                }
            }
        }
        sideToMove=opposite(sideToMove);
        
    }
    void Board::unmakeMove(){
        Move prev=moveHistory.back();
        moveHistory.pop_back();
        castlingRights=history.back().castlingRights;
        enpassentSquare=history.back().enPassantSquare;
        sideToMove=opposite(sideToMove);
        bool isBlack= sideToMove==Piece::Black;
        history.pop_back();
        squares[prev.from]=prev.movedpiece;
        if(!prev.isEnPassant) squares[prev.to]=prev.capturedpiece;
        if(prev.isCastle){
            int castleFile=prev.to%8;
            if(castleFile==6){
                if(isBlack){
                    squares[63]=Piece::Black | Piece::Rook;
                    squares[61]=Piece::None;
                }
                else{
                    squares[7]=Piece::White | Piece::Rook;
                    squares[5]=Piece::None;
                }
            }else{
                if(isBlack){
                    squares[56]=Piece::Black | Piece::Rook;
                    squares[59]=Piece::None;
                }
                else{
                    squares[0]=Piece::White | Piece::Rook;
                    squares[3]=Piece::None;
                }
            }
        }
        if(prev.isEnPassant){
            squares[prev.to] = Piece::None;
            if (!isBlack)
                squares[prev.to - 8] = prev.capturedpiece;
            else
                squares[prev.to + 8] = prev.capturedpiece;
        }
    }