#include <chrono>
#include <iostream>

#include "MoveSort.h"
#include "direction.h"
#include "engine.h"
#include "evaluate.h"
#include "move.h"
#include "movegen.h"
#include "nnue/nnue.h"
#include "perft.h"
#include "search.h"
#include "square.h"
#include "time.h"
#include "transposition.h"
#include <cstring>
#include "platform.h"

Engine::Engine()
{
    initSquare();
    initDirection();
    initBBs();
    InitZobrist();
    InitMagics();

    std::memset(moveHistory, 0, sizeof(moveHistory));


    std::string exeDir;
    GetExecutablePath(exeDir);
    exeDir = exeDir.substr(0, exeDir.find_last_of("/\\"));
    bool nnueLoaded = nnue->Load(exeDir + "/nnue_bin/nnue01.bin");
    if (!nnueLoaded)
    {
        std::cerr << "Failed to load NNUE network." << std::endl;
    }

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

    DirtyMove dirtyMove;
    for (Move* mPtr = legal.moves; mPtr < legal.end; mPtr++)
    {
        Move m = *mPtr;
        if (m.to() == move.to() && m.from() == move.from() &&
            ((m.type() == PROMOTION && move.promotion() == m.promotion()) || (m.type() != PROMOTION)))
        {
            board->makeMove(m, &states[board->getPly() + 1], dirtyMove);
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
    DirtyMove dirtyMove;

    for (Move* mPtr = moves.moves; mPtr < moves.end; mPtr++)
    {
        Move move = *mPtr;
        board->makeMove(move, &state, dirtyMove);
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
    Accumulator white, black;
    board->ResetWhiteAccumulator(white);
    board->ResetBlackAccumulator(black);
    Accumulator& us = board->whiteToMove ? white : black;
    Accumulator& them = board->whiteToMove ? black : white;
    Score score = nnue->Evaluate(*board, us, them) * (board->whiteToMove ? 1 : -1);
    Score psqt = nnue->FastEvaluate(*board, us, them) * (board->whiteToMove ? 1 : -1);
    std::cout << "Eval: " << score << std::endl;
    std::cout << "Positional: " << score - psqt << std::endl;
    std::cout << "Psqt: " << psqt << std::endl;
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