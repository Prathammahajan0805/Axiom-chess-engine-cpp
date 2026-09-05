#include"board.h"
#include "attacks.h"
#include"bitboards.h"
#include "search.h"
#include "types.h"
#include "zobrist.h"
#include"evaluation.h"
#include<sstream>
#include"TT.h"
using namespace std;

int charToPiece(char c){
    int color = isupper(c) ? WHITE : BLACK;
    switch(tolower(c)){
        case 'p': return color==WHITE? WP:BP;
        case 'r': return color==WHITE? WR:BR;
        case 'q': return color==WHITE? WQ:BQ;
        case 'n': return color==WHITE? WN:BN;
        case 'b': return color==WHITE? WB:BB;
        case 'k': return color==WHITE? WK:BK;
    }
    return 0;
}


void Board::clearBoard() {
    for (int i = 0; i < 12; i++)
        pieces[i] = 0ULL;
    std::fill(pieceOn.begin(), pieceOn.end(), NO_PIECE);
    whitePieces = 0ULL;
    blackPieces = 0ULL;
    occupied = 0ULL;

    sideToMove = WHITE;
    castlingRights = 0;
    enPassantSquare = NO_SQUARE;
    hash=pawnHash=0ULL;
    whiteKingSquare = NO_SQUARE;
    blackKingSquare = NO_SQUARE;
    ply=0;
}
void Board::loadFEN(const string& fen) {
        
        clearBoard();
        std::stringstream ss(fen);
        std::string boardPart;
        std::string sidePart;
        std::string castlePart;
        std::string epPart;
        int halfmove, fullmove;

        ss >> boardPart >> sidePart >> castlePart >> epPart >> halfmove >> fullmove;

        // ---------------- Piece placement ----------------

        int rank = 7;
        int file = 0;

        for (char c : boardPart) {

            if (c == '/') {
                rank--;
                file = 0;
                continue;
            }

            if (isdigit(c)) {
                file += c - '0';
                continue;
            }

            
            Piece piece = (Piece) charToPiece(c);
            setBit(pieces[piece],getSquare(file, rank));
            pieceOn[getSquare(file, rank)]=piece;
            if(piece == WK)
                whiteKingSquare = getSquare(file,  rank);
            if(piece == BK)
                blackKingSquare = getSquare(file,  rank);
            hash^=randomForZobrist[piece][getSquare(file,rank)];
            if(piece==WP || piece==BP){
                pawnHash ^= randomForZobrist[piece][getSquare(file,rank)];
            }

            file++;
        }

        // ---------------- Side to move ----------------

        sideToMove = (sidePart == "w") ? WHITE : BLACK;
        if(sideToMove==BLACK){
            hash^=randomForSide;
        }

        // ---------------- Castling ----------------

        castlingRights = 0;

        if (castlePart.find('K') != std::string::npos)
            castlingRights |= Castling::WHITE_KINGSIDE;

        if (castlePart.find('Q') != std::string::npos)
            castlingRights |= Castling::WHITE_QUEENSIDE;

        if (castlePart.find('k') != std::string::npos)
            castlingRights |= Castling::BLACK_KINGSIDE;

        if (castlePart.find('q') != std::string::npos)
            castlingRights |= Castling::BLACK_QUEENSIDE;
        hash^=randomForCastling[castlingRights];
        // ---------------- En passant ----------------

        if (epPart == "-") {
            enPassantSquare = NO_SQUARE;
        }
        else {
            int epfile = epPart[0] - 'a';
            int eprank = epPart[1] - '1';
            enPassantSquare = getSquare(epfile, eprank);
            hash^=randomForEnPassant[epfile];
        }
        updateOccupancies();
        
}
void Board::print(Bitboard bb) const{
    string s="\n  --------------------------------\n";
    for (int rank = RANKV_8; rank >= RANKV_1; rank--) {
        for (int file = FILEV_A; file <= FILEV_H; file++) {
            if(file==FILEV_A) s+=to_string(rank+1)+" ";
            if(getBit(bb, getSquare(file, rank))) s+= "| "+pieceToUnicode((Piece) pieceOn[getSquare(file, rank)])+" ";
            else s+="|   ";
        }
        s+="| \n  --------------------------------\n";
        
    }
    s+="    a   b   c   d   e   f   g   h \n";
    cout<<s;
    }
    string squareToString(int sq){
    string s="";
    int rank=sq/8;
    int file=sq%8;
    s+=static_cast<char>(file+'a');
    s+=to_string(rank+1);
    return s;
}

string Board::toFEN() const {
        string fen = "";
        
        for (int rank = 7; rank >= 0; rank--) {
            int emptyCount = 0;
            
            for (int file = 0; file < 8; file++) {
                Piece piece = pieceOn[getSquare(file, rank)];
                
                if (piece == NO_PIECE) {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) {
                        fen += to_string(emptyCount);
                        emptyCount = 0;
                    }
                    
                    char p = pieceToChar(piece);
                    if (piece>=WP && piece<=WK) {
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
Board::Board(){
    pieceOn.fill(NO_PIECE);
    hash=pawnHash=0ULL;
    halfMoveClock=0;
    loadFEN(startingFen);
    initLeaperAttacks();
    initLineBetween();
    initAllSliderAttacks();
    initShifts();
    initZobrist();
    initEval();
    initSplats();
    initPassedPawnsMask();
    initFileMasks();
    initSearch();
    initLMR();
}