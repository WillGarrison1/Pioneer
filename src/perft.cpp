#include "perft.h"
#include "movegen.h"

unsigned long long perft(Board& board, unsigned int depth)
{

    MoveList moves;
    board.generateMoves<ALL_MOVES>(&moves);

    if (depth == 1)
    {
        return moves.size;
    }
    else if (depth == 0)
    {
        return 1;
    }

    unsigned long long moveCount = 0;

    BoardState state;

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];
        board.makeMove(move, &state);
        moveCount += perft(board, depth - 1);
        board.undoMove();
    }

    return moveCount;
}