#include "time.h"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <thread>

#include "search.h"

#include "MoveSort.h"
#include "evaluate.h"

#define INF 1000000
#define MATE 100000

#define MAX_DEPTH 256
#define FUTILITY_DEPTH 4
#define LMR_INDEX 4
#define LMR_DEPTH 3
#define FUTILITY_MARGIN(DEPTH) 50 + 100 * DEPTH

unsigned long long numNodes;
unsigned long long maxNodes;
unsigned long long maxTime;
unsigned long long startTime;

bool isDone;

struct PVLine
{
    int len;
    Move moves[MAX_DEPTH];

    PVLine() : len(0)
    {
    }
};

Score bestScore = -INF;
Move bestMove = 0;

std::string GetMoveListString(PVLine* l)
{
    std::string moves;
    for (int i = 0; i < l->len; i++)
    {
        moves += l->moves[i].toString() + " ";
    }

    return moves;
}

Score qsearch(Board& board, int ply, Score alpha, Score beta, PVLine* prevLine)
{
    prevLine->len = 0;

    numNodes++;
    Score pat = Eval(board);

    if (pat >= beta)
    {
        return pat;
    }

    if (alpha < pat)
        alpha = pat;

    // Generate moves

    MoveList moves;
    if (!board.getNumChecks()) // If not in check, generate captures
    {
        generateMoves<CAPTURE>(board, &moves);
        if (!moves.size) // if no moves, check for stalemate
        {
            generateMoves<ALL_MOVES>(board, &moves);
            if (moves.size == 0)
                return 0; // stalemate
            return pat;
        }
    }
    else // if in check, generate evasions
    {
        generateMoves<ALL_MOVES>(board, &moves);
        if (!moves.size) // if no moves, checkmate
        {
            return -MATE + ply;
        }
    }

    SortMovesQ(board, &moves);

    PVLine line;
    for (int i = 0; i < moves.size; i++)
    {
        Move m = moves.moves[i];

        board.makeMove(m);
        Score score = -qsearch(board, ply + 1, -beta, -alpha, &line);
        board.undoMove();

        if (score >= beta)
        {
            prevLine->moves[0] = m;
            prevLine->len = 1;
            return score;
        }
        if (score > alpha)
            alpha = score;
        if (score > pat)
        {
            pat = score;
            prevLine->moves[0] = m;
            memcpy(prevLine->moves + 1, &line.moves, line.len * sizeof(Move));
            prevLine->len = line.len + 1;
        }
    }

    return pat;
}

Score search(Board& board, int depth, int ply, Score alpha, Score beta, PVLine* prevLine)
{
    if (isDone)
        return alpha;

    unsigned long long time = getTime();

    if (time - startTime > maxTime)
    {
        isDone = true;
        return alpha; //! Maybe return 0 here
    }

    if (numNodes > maxNodes)
    {
        isDone = true;
        return alpha;
    }

    if (depth == 0)
    {
        return qsearch(board, ply, alpha, beta, prevLine);
    }

    prevLine->len = 0;

    numNodes++;

    MoveList moves;
    generateMoves<ALL_MOVES>(board, &moves);

    if (board.getRepetitions() == 6)
        return 0; // draw by repetition

    if (moves.size == 0)
    {
        if (board.getNumChecks()) // if in check, then checkmate
            return -MATE + ply;
        else // else, stalemate
            return 0;
    }

    SortMoves(board, &moves);

    PVLine line;

    Score best = -INF;

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];

        // Late move reductions (LMR)

        int newDepth = depth - 1;
        // if (depth >= LMR_DEPTH && i > LMR_INDEX && move.type() == QUIET && !board.getNumChecks() &&
        // !board.isCheckMove(move))
        //     newDepth--;

        board.makeMove(move);

        // Futility pruning
        // if (depth < FUTILITY_DEPTH && move.type() == QUIET && !board.getNumChecks())
        // {
        //     Score eval = Eval(board) + FUTILITY_MARGIN(depth);
        //     if (eval <= alpha)
        //     {
        //         board.undoMove();
        //         continue;
        //     }
        // }

        // Depth Extensions/reductions
        if (board.getNumChecks()) // check extension
            newDepth++;

        Score score = -search(board, std::max(newDepth, 0), ply + 1, -beta, -alpha, &line);
        board.undoMove();

        if (isDone)
            break;

        if (score >= beta)
        {
            prevLine->moves[0] = move;
            prevLine->len = 1;
            return score;
        }
        if (score > alpha)
            alpha = score;

        if (score > best)
        {
            best = score;
            prevLine->moves[0] = move;
            memcpy(prevLine->moves + 1, &line.moves, line.len * sizeof(Move));
            prevLine->len = line.len + 1;
        }
    }

    return best;
}

Score iterativeDeepening(Board& board, unsigned int depth, unsigned int nodes, unsigned int movetime)
{
    startTime = getTime();

    PVLine pv;
    pv.len = 0;

    Move prevBestMove;
    Score prevBestScore;

    bestMove = 0;
    bestScore = -INF;
    numNodes = 0;

    if (!movetime)
        movetime = -1;
    if (!nodes)
        nodes = -1;
    if (!depth)
        depth = MAX_DEPTH;

    maxTime = movetime;
    maxNodes = nodes;

    for (unsigned int d = 1; d <= depth; d++)
    {
        PVLine line;
        line.len = 0;

        bestMove = 0;
        bestScore = -INF;

        MoveList m;
        generateMoves<ALL_MOVES>(board, &m);

        SortMoves(board, &m, prevBestMove);

        for (int i = 0; i < m.size; i++)
        {
            board.makeMove(m.moves[i]);
            Score eval = -search(board, d - 1, 1, -INF, INF, &line);
            board.undoMove();

            if (isDone)
            {
                bestScore = prevBestScore;
                bestMove = prevBestMove;
                return bestScore;
            }

            if (eval > bestScore)
            {
                bestScore = eval;
                bestMove = m.moves[i];

                pv.moves[0] = bestMove;
                memcpy(pv.moves + 1, &line.moves, line.len * sizeof(Move));
                pv.len = line.len + 1;

                std::cout << "info depth " << d << " curmov " << bestMove.toString() << " score cp " << bestScore
                          << " nodes " << numNodes << " pv " << GetMoveListString(&pv) << "time "
                          << getTime() - startTime << std::endl;
            }
        }

        std::cout << "info depth " << d << " curmov " << bestMove.toString() << " score cp " << bestScore << " nodes "
                  << numNodes << " pv " << GetMoveListString(&pv) << std::endl;

        prevBestMove = bestMove;
        prevBestScore = bestScore;
    }

    return bestScore;
}

Move startSearch(Board& board, unsigned int depth, unsigned int nodes, unsigned int movetime)
{
    isDone = false;

    Score eval = iterativeDeepening(board, depth, nodes, movetime);

    std::cout << "bestmove " << bestMove.toString() << " score cp " << eval << std::endl;

    return bestMove;
}