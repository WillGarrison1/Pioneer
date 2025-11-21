#include "perft.h"
#include "movegen.h"

unsigned long long perft(Board &board, unsigned int depth)
{

    MoveList moves;
    generateMoves<ALL_MOVES>(board, &moves);

    if (depth == 1)
    {
        return moves.size;
    }
    else if (depth == 0)
    {
        return 1;
    }

    unsigned long long moveCount = 0;

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];
        board.makeMove(move);
        moveCount += perft(board, depth - 1);
        board.undoMove();
    }

    return moveCount;
}