#include "uci.h"
#include "TT.h"
#include "board.hpp"
#include "movegen.h"
#include "search.h"
#include"moves.h"
#include"makeMove.h"

#include "types.h"
#include <sstream>
#include <string>
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

static const std::string STARTPOS_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
bool ponder=false;
GoParams p;

// ===== multithreading state =====
static int numThreads = 1;
static SharedState shared;
static std::vector<SearchThread> threadContexts;
static std::thread dispatcherThread;

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

// join all worker threads to main
void joinSearch(){
    if(dispatcherThread.joinable()) dispatcherThread.join();
}

// launch worker threads if any + main thread
void launchSearch(int depth, uint64_t timeMs, Board& board){
    joinSearch();
    shared.stopFlag.store(false);
    shared.stopSearch.store(false);
    shared.startTime = std::chrono::steady_clock::now();
    shared.timeLimitMs = timeMs;
    shared.optimalTimeMs = timeMs;
    shared.softLimitMs.store((int64_t)timeMs);

    Board boardSnapshot = board;
    int depthCopy = depth;

    dispatcherThread = std::thread([boardSnapshot, depthCopy](){
        threadContexts.clear();
        threadContexts.resize(numThreads);

        std::vector<std::thread> workers;
        workers.reserve(numThreads);

        for (int i = 0; i < numThreads; i++){
            threadContexts[i].id = i;
            initSearchThread(threadContexts[i], boardSnapshot);
            workers.emplace_back(threadLoop, std::ref(threadContexts[i]), std::ref(shared), depthCopy);
        }

        for (auto& w : workers){
            if (w.joinable()) w.join();
        }

        SearchThread& best = pickBestThread(threadContexts);

        std::string output = "bestmove " + moveToUci(best.bestMove);
        if (best.pvLength[0] >= 2){
            output += " ponder " + moveToUci(best.pvTable[0][1]);
        }
        std::cout << output << std::endl;
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
        budget = std::min(budget*2.0, remaining *0.8);
        if (budget < 10) budget = 10;

        depthOut = (p.depth > 0) ? p.depth : MAX_SEARCH_DEPTH;
        timeMsOut = budget;
        return;
    }
    depthOut = MAX_SEARCH_DEPTH;
    timeMsOut = HUGE_TIME_MS;
}

GoParams parseGo(const std::vector<std::string>& tokens) {
    p = GoParams();
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size())      p.depth = stoi(tokens[++i]);
        else if (tokens[i] == "movetime" && i + 1 < tokens.size())p.movetime = stoll(tokens[++i]);
        else if (tokens[i] == "wtime" && i + 1 < tokens.size())p.wtime = stoll(tokens[++i]);
        else if (tokens[i] == "btime" && i + 1 < tokens.size())p.btime = stoll(tokens[++i]);
        else if (tokens[i] == "winc" && i + 1 < tokens.size())p.winc = stoll(tokens[++i]);
        else if (tokens[i] == "binc" && i + 1 < tokens.size())p.binc = stoll(tokens[++i]);
        else if (tokens[i] == "movestogo" && i + 1 < tokens.size())p.movestogo= stoi(tokens[++i]);
        else if (tokens[i] == "infinite") p.infinite = true;
        else if (tokens[i] == "ponder") ponder=true;
    }
    return p;
}

//position [fen <fenstring>] [startpos] moves <move1>...<moven>
void applyPositionCommand(const vector<string>& tokens, Board& board){
    size_t ind=1;
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

void applySetOption(const vector<string>& tokens){
    if (tokens.size() < 5) return;
    if (tokens[1] != "name") return;

    string name = tokens[2];
    // value token index: find "value"
    size_t valueIdx = 0;
    for (size_t i = 3; i < tokens.size(); i++){
        if (tokens[i] == "value" && i + 1 < tokens.size()){
            valueIdx = i + 1;
            break;
        }
    }
    if (valueIdx == 0) return;

    if (name == "Threads"){
        joinSearch();
        int n = stoi(tokens[valueIdx]);
        numThreads = std::max(1, n);
    } else if (name == "Hash"){
        joinSearch();
        int mb = stoi(tokens[valueIdx]);
        resizeTT(mb);
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
            cout << "option name Threads type spin default 1 min 1 max 256\n";
            cout << "option name Hash type spin default 16 min 1 max 4096\n";
            cout << "option name Ponder type check default false\n";
            cout << "uciok"<<endl;
        }
        else if(cmd=="isready"){
            cout<<"readyok"<<endl;
        }
        else if(cmd=="ucinewgame"){
            shared.stopFlag.store(true);
            joinSearch();
            board.loadFEN(STARTPOS_FEN);
            clearTT();
            threadContexts.clear();
        }else if(cmd=="position"){
            shared.stopFlag.store(true);
            shared.stopSearch.store(true);
            joinSearch();
            applyPositionCommand(tokens,board);
        }else if(cmd=="go"){
            joinSearch();
            p=parseGo(tokens);
            int depth;
            int64_t timeMs;
            computeSearchBudget(p,Color(board.sideToMove), depth, timeMs);
            shared.isPondering.store(ponder);
            launchSearch(depth, timeMs, board);
        }
        else if(cmd=="setoption"){
            applySetOption(tokens);
        }
        else if(cmd=="stop"){
            shared.stopSearch.store(true);
            shared.stopFlag.store(true);
            joinSearch();
        }
        else if(cmd=="quit"){
            shared.stopFlag.store(true);
            joinSearch();
            break;
        }else if(cmd=="ponderhit"){
            shared.isPondering.store(false);
        }
    }
}