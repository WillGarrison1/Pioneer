#include <chrono>
#include <iostream>

#include "MoveSort.h"
#include "direction.h"
#include "engine.h"
#include "evaluate.h"
#include "move.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include "square.h"
#include "time.h"
#include "transposition.h"
#include <cstring>

Engine::Engine()
{
    initSquare();
    initDirection();
    initBBs();
    InitZobrist();
    InitMagics();

    std::memset(moveHistory, 0, sizeof(moveHistory));
    std::memset(killerMoves, 0, sizeof(killerMoves));

    board = new Board;
    board->setFen(START_FEN, &states[0]);
}

Engine::~Engine()
{
    delete board;
}

void Engine::makemove(Move move)
{
    MoveList legal;
    board->generateMoves<ALL_MOVES>(&legal);

    for (Move* mPtr = legal.moves; mPtr < legal.end; mPtr++)
    {
        Move m = *mPtr;
        if (m.to() == move.to() && m.from() == move.from() &&
            ((m.type() == PROMOTION && move.promotion() == m.promotion()) || (m.type() != PROMOTION)))
        {
            board->makeMove(m, &states[board->getPly() + 1]);
            break;
        }
    }
}

void Engine::go(unsigned int depth, unsigned int nodes, unsigned int movetime, unsigned int wtime, unsigned int btime)
{
    unsigned int remaining_time = board->whiteToMove ? wtime : btime;
    startSearch(*board, depth, nodes, movetime, remaining_time);
}

void Engine::goPerft(unsigned int depth)
{
    unsigned long long start = getTime();
    unsigned long long moveCount = 0;

    MoveList moves;
    board->generateMoves<ALL_MOVES>(&moves);
    BoardState state;

    for (Move* mPtr = moves.moves; mPtr < moves.end; mPtr++)
    {
        Move move = *mPtr;
        board->makeMove(move, &state);
        unsigned long long count = perft(*board, depth - 1);
        board->undoMove();

        moveCount += count;

        std::cout << move.toString() << ": " << count << "\n";
    }

    unsigned long long end = getTime();
    std::cout << "Total Moves: " << moveCount << " Took: " << (end - start) << " ms" << std::endl;
}

void Engine::eval()
{
    Score score = Eval<FULL>(*board) * (board->whiteToMove ? 1 : -1);
    std::cout << "Eval: " << score << std::endl;
    std::cout << "Pawns: " << board->getPawnMaterial() << std::endl;
    std::cout << "Other: " << board->getNonPawnMaterial() << std::endl;
}

void Engine::isCheck(Move move)
{
    MoveList legal;
    board->generateMoves<ALL_MOVES>(&legal);

    for (Move* mPtr = legal.moves; mPtr < legal.end; mPtr++)
    {
        Move m = *mPtr;
        if (m.to() == move.to() && m.from() == move.from() && m.promotion() == move.promotion())
        {
            std::cout << board->isCheckMove(m) << std::endl;
            break;
        }
    }
}