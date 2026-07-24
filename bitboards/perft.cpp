#include "perft.h"
#include "moves.h"
#include "types.h"
#include <cstdint>

uint64_t perft(Board& board, int depth) {
    if (depth == 0)
        return 1;
    MoveList moves=generateMoves<ALL>( board);
    if (depth == 1) return moves.count;

    uint64_t nodes = 0;

    for (int i = 0; i < moves.count; i++) {
        makeMove(board, moves.moves[i]);
        nodes += perft(board, depth - 1);
        unMakeMove(board);
    }

    return nodes;
}

uint64_t perftRoot(Board& board, int depth) {
    MoveList moves=generateMoves<ALL>(board);

    uint64_t total = 0;

    for (int i = 0; i < moves.count; i++) {
        makeMove(board, moves.moves[i]);

        uint64_t cnt = perft(board, depth - 1);

        unMakeMove(board);

        std::cout << printMove(moves.moves[i])
                  << ": "
                  << cnt
                  << '\n';

        total += cnt;
    }

    std::cout << "Nodes: " << total << '\n';
    return total;
}