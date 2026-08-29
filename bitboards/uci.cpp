#include"uci.h"
#include "TT.h"
#include "board.h"
#include "search.h"
#include "types.h"
#include<sstream>
#include <string>
#include<cmath>
#include <thread>
static const std::string STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
std::thread searchThread;
using namespace std;
vector<string> splitTokens(string line){
    vector<string> tokens;
    stringstream ss(line);
    string word;
    while(ss>>word) tokens.push_back(word);
    return tokens;
        
}
//move<->uci
std::string moveToUci(const Move& move) {
    if(move.moveInfo==0) return "0000";
    std::string s = squareToString(getFrom(move)) + squareToString(getTo(move));
    int promo = getPromotionPiece(move);
    if (promo != NO_PIECE) {
        s += (char)tolower(pieceToChar((Piece)promo));
    }
    return s;
}
void joinSearch(){
    if(searchThread.joinable()) searchThread.join();
}
void launchSearch(int depth,uint64_t timeMs,Board& board){
    joinSearch();
    searchThread=std::thread([&board,depth,timeMs](){
        Move best=findBestMove(board, depth,timeMs);
        std::cout<<"bestmove "<<moveToUci(best)<<std::endl;
    });
}
Move uciToMove(const std::string& moveStr, Board& board) {
    if (moveStr.size() < 4) return Move{0};
 
    int fromFile = moveStr[0] - 'a';
    int fromRank = moveStr[1] - '1';
    int toFile   = moveStr[2] - 'a';
    int toRank   = moveStr[3] - '1';
    if (fromFile < 0 || fromFile > 7 || fromRank < 0 || fromRank > 7 ||
        toFile   < 0 || toFile   > 7 || toRank   < 0 || toRank   > 7) {
        return Move{0};
    }
 
    int from = getSquare(fromFile, fromRank);
    int to   = getSquare(toFile, toRank);
    char promoChar = (moveStr.size() > 4) ? (char)tolower(moveStr[4]) : '\0';
    //cross verify
    MoveList moves = generateMoves<ALL>(board);
    for (int i = 0; i < moves.count; i++) {
        Move& m = moves.moves[i];
        if (getFrom(m) != from || getTo(m) != to) continue;
 
        int promo = getPromotionPiece(m);
        if (promoChar == '\0') {
            if (promo == NO_PIECE) return m;
        } else {
            if (promo == NO_PIECE) continue;
            if (tolower(pieceToChar((Piece)promo)) == promoChar) return m;
        }
    }
    return Move{0}; // illegal
}
//due depth
static constexpr int MAX_SEARCH_DEPTH = 64;
static constexpr int64_t HUGE_TIME_MS = 1LL << 40;
 

void computeSearchBudget(const GoParams& p, Color side, int& depthOut, int64_t& timeMsOut) {
    if (p.infinite) {
        depthOut = MAX_SEARCH_DEPTH;
        timeMsOut = HUGE_TIME_MS;
        return;
    }
    if (p.movetime >= 0) {
        depthOut = MAX_SEARCH_DEPTH;
        timeMsOut = p.movetime;
        return;
    }
    if (p.depth > 0 && p.wtime < 0 && p.btime < 0) {
        depthOut = p.depth;
        timeMsOut = HUGE_TIME_MS;
        return;
    }
    if (p.wtime >= 0 || p.btime >= 0) {
        int64_t remaining = (side == WHITE) ? p.wtime : p.btime;
        int64_t inc       = (side == WHITE) ? p.winc  : p.binc;
        int movesToGo     = (p.movestogo > 0) ? p.movestogo : 30;
 
        int64_t budget = remaining / movesToGo + (inc * 8) / 10; 
        budget = std::min(budget, remaining - 50);
        if (budget < 10) budget = 10;
 
        depthOut = (p.depth > 0) ? p.depth : MAX_SEARCH_DEPTH;
        timeMsOut = budget;
        return;
    }
    depthOut = MAX_SEARCH_DEPTH;
    timeMsOut = HUGE_TIME_MS;
}
GoParams parseGo(const std::vector<std::string>& tokens) {
    GoParams p;
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size())      p.depth = stoi(tokens[++i]);
        else if (tokens[i] == "movetime" && i + 1 < tokens.size())p.movetime = stoll(tokens[++i]);
        else if (tokens[i] == "wtime" && i + 1 < tokens.size())p.wtime = stoll(tokens[++i]);
        else if (tokens[i] == "btime" && i + 1 < tokens.size())p.btime = stoll(tokens[++i]);
        else if (tokens[i] == "winc" && i + 1 < tokens.size())p.winc = stoll(tokens[++i]);
        else if (tokens[i] == "binc" && i + 1 < tokens.size())p.binc = stoll(tokens[++i]);
        else if (tokens[i] == "movestogo" && i + 1 < tokens.size())p.movestogo= stoi(tokens[++i]);
        else if (tokens[i] == "infinite") p.infinite = true;
    }
    return p;
}


//position [fen <fenstring>] [startpos] moves <move1>...<moven>
void applyPositionCommand(const vector<string>& tokens,Board& board){
    uint8_t ind=1;
    if(ind>tokens.size()) return;
    if(tokens[ind]=="startpos"){
        board.loadFEN(STARTPOS_FEN);
        ind++;
    }else if(tokens[ind]=="fen"){
        ind++;
        string fen;
        while(ind<tokens.size() && tokens[ind]!="moves"){
            if(!fen.empty()) fen+=" ";
            fen+=tokens[ind];
            ind++;
        }
        board.loadFEN(fen);
    }else{
        return;
    }
    if(ind<tokens.size() && tokens[ind]=="moves"){
        ind++;
        for (; ind < tokens.size(); ind++) {
            Move m = uciToMove(tokens[ind], board);
            if (m.moveInfo == 0) break;
            makeMove(board, m);
        }

    }
}
void initUCI(){
    Board board;
    string line;
    while(getline(cin,line)){
        vector<string> tokens=splitTokens(line);
        if(tokens.empty()) continue;
        const string cmd=tokens.front();
        if(cmd=="uci"){
            cout << "id name \n";
            cout << "id author Pratham Mahajan\n";
            cout << "uciok"<<endl;
        }
        else if(cmd=="isready"){
            cout<<"readyok"<<endl;
        }
        else if(cmd=="ucinewgame"){
            joinSearch();
            board.loadFEN(STARTPOS_FEN);
            initSearch();
        }else if(cmd=="position"){
            joinSearch();
            applyPositionCommand(tokens,board);
        }else if(cmd=="go"){
            joinSearch();
            GoParams p=parseGo(tokens);
            int depth;
            int64_t timeMs;
            computeSearchBudget(p,Color(board.sideToMove), depth, timeMs);
            launchSearch(depth, timeMs, board);
        }
        else if(cmd=="setoptions"){
            //to do
        }
        else if(cmd=="stop"){
            stopSearch();
            joinSearch();
        }
        else if(cmd=="quit") break;

    }
}