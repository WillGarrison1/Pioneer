#include <chrono>
#include <iostream>

#include "direction.h"
#include "engine.h"
#include "evaluate.h"
#include "move.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include "square.h"

double getTime()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

Engine::Engine()
{
    initSquare();
    initDirection();
    initBBs();
}

Engine::~Engine()
{
}

void Engine::makemove(Move move)
{
    MoveList legal;
    generateMoves<ALL_MOVES>(board, &legal);

    for (int i = 0; i < legal.size; i++)
    {
        Move m = legal.moves[i];
        if (m.to() == move.to() && m.from() == move.from() && m.promotion() == move.promotion())
        {
            board.makeMove(m);
            break;
        }
    }
}

void Engine::go(unsigned int depth)
{
    Move best = startSearch(board, depth);
    std::cout << "Best Move: " << best.toString() << std::endl;
}

void Engine::goPerft(unsigned int depth)
{
    double start = getTime();
    unsigned long long moveCount = 0;

    MoveList moves;
    generateMoves<ALL_MOVES>(board, &moves);

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];
        board.makeMove(move);
        unsigned long long count = perft(board, depth - 1);
        board.undoMove();

        moveCount += count;

        std::cout << move.toString() << ": " << count << "\n";
    }
    double end = getTime();
    std::cout << "Total Moves: " << moveCount << " Took: " << (end - start) / 1000000000.0 << std::endl;
}

void Engine::eval()
{
    Score score = Eval(board) * (board.whiteToMove ? 1 : -1);
    std::cout << "Eval: " << score << std::endl;
    std::cout << "Pawns: " << board.getPawnMaterial() << std::endl;
    std::cout << "Other: " << board.getNonPawnMaterial() << std::endl;
}

void Engine::isCheck(Move move)
{
    MoveList legal;
    generateMoves<ALL_MOVES>(board, &legal);

    for (int i = 0; i < legal.size; i++)
    {
        Move m = legal.moves[i];
        if (m.to() == move.to() && m.from() == move.from() && m.promotion() == move.promotion())
        {
            std::cout << board.isCheckMove(m) << std::endl;
            break;
        }
    }
}