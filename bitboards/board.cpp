#include"board.h"
#include "attacks.h"
#include"bitboards.h"
#include<sstream>
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
char pieceToChar(Piece piece){
    switch(piece){
        case WP: return 'P';break;
        case WB: return 'B';break;
        case WN: return 'N';break;
        case WR: return 'R';break;
        case WQ: return 'Q';break;
        case WK: return 'K';break;
        case BP: return 'p';break;
        case BB: return 'b';break;
        case BN: return 'n';break;
        case BR: return 'r';break;
        case BQ: return 'q';break;
        case BK: return 'k';break;
        case NO_PIECE: return '\0';break;
    }
    return '\0';
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

    whiteKingSquare = NO_SQUARE;
    blackKingSquare = NO_SQUARE;
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
            

            file++;
        }

        // ---------------- Side to move ----------------

        sideToMove = (sidePart == "w") ? WHITE : BLACK;

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

        // ---------------- En passant ----------------

        if (epPart == "-") {
            enPassantSquare = NO_SQUARE;
        }
        else {
            int epfile = epPart[0] - 'a';
            int eprank = epPart[1] - '1';
            enPassantSquare = getSquare(epfile, eprank);
        }
        updateOccupancies();
}
void Board::print() const{
    displayBitboard(occupied);
}
Board::Board(){
    pieceOn.fill(NO_PIECE);
    loadFEN(startingFen);
    initLeaperAttacks();
    initLineBetween();
    initAllSliderAttacks();
    initShifts();
}