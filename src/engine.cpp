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
#include "platform.h"
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

    std::string exeDir;
    GetExecutablePath(exeDir);
    exeDir = exeDir.substr(0, exeDir.find_last_of("/\\"));
    bool nnueLoaded = nnue->Load(exeDir + "/nnue_bin/nnue02.bin");
    if (!nnueLoaded)
    {
        std::cerr << "Failed to load NNUE network." << std::endl;
    }

    board = new Board;
    board->setFen(START_FEN, &states[0]);

    searcher = new Searcher;
}

Engine::~Engine()
{
    delete board;
    delete searcher;
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
    SearchConstraints constraints;
    constraints.maxDepth = depth;
    constraints.maxNodes = nodes;
    constraints.movetime = movetime;
    constraints.remainingTime = remaining_time;
    searcher->StartSearch(*board, constraints);
}

void Engine::stop()
{
    searcher->Stop();
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
    float score = nnue->Evaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);
    float psqt = nnue->FastEvaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);

    for (Rank r = RANK_8; r >= RANK_1; r = (Rank)(r - 1))
    {
        std::string rank[5] = {"+", "|", "|", "|", "|"};
        for (File f = FILE_A; f <= FILE_H; f = (File)(f + 1))
        {
            Square s = getSquare(f, r);
            Piece p = board->getSQ(s);

            if (p == EMPTY)
            {
                rank[0] += "-------+";
                rank[1] += "       |";
                rank[2] += "       |";
                rank[3] += "       |";
                rank[4] += "       |";
                continue;
            }

            float pieceVal = 0.0f;
            if (getType(p) != KING)
            {
                board->removePiece(s);

                board->ResetWhiteAccumulator(white);
                board->ResetBlackAccumulator(black);
                float newScore = nnue->Evaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);
                pieceVal = (score - newScore) / 100;

                board->addPiece(p, s);
            }

            char buffer[7];
            std::snprintf(buffer, sizeof(buffer), "%.1f", pieceVal);

            std::string value(buffer);
            if (value.length() < 7)
            {
                int dif = 7 - value.length();
                int leftZ = std::ceil(dif / 2.0f);
                int rightZ = std::floor(dif / 2.0f);
                value.insert(value.begin(), leftZ, ' ');
                value.insert(value.end(), rightZ, ' ');
            }

            rank[0] += "-------+";
            rank[1] += "       |";
            rank[2] += "   "+pieceToString(p)+"   |";
            rank[3] += value+"|";
            rank[4] += "       |";
        }
        std::cout << rank[0] << "\n" << rank[1] << "\n" << rank[2] << "\n" << rank[3] << "\n" << rank[4] << "\n";
    }
    std::cout << "+-------+-------+-------+-------+-------+-------+-------+-------+\n\n";
    std::cout << "Positional: " << score - psqt << "\nPsqt: " << psqt << "\n\nEval: " << score << std::endl;
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