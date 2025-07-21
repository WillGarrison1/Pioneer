#include "time.h"
#include <cmath>
#include <cstring>
#include <iostream>

#include "search.h"

#include "MoveSort.h"
#include "evaluate.h"
#include "transposition.h"

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
static TranspositionTable* tTable =
    new TranspositionTable(1024 * 1024 * 256); // transposition table with a size of 64 MB

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

Score qsearch(Board& board, int ply, Score alpha, Score beta)
{
    numNodes++;

    // probe tt

    TranspositionEntry* entry = tTable->GetEntry(board.getHash());
    if (entry && entry->depth >= 0)
    {
        if (entry->nodeType == NodeType::Exact)
        {
            return entry->score;
        }
        if (entry->nodeType == NodeType::Upper && entry->score <= alpha)
        {
            return entry->score;
        }
        if (entry->nodeType == NodeType::Lower && entry->score >= beta)
        {
            return entry->score;
        }
    }

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
        Score score = -qsearch(board, ply + 1, -beta, -alpha);
        board.undoMove();

        if (score >= beta)
        {
            return score;
        }
        if (score > alpha)
            alpha = score;
        if (score > pat)
        {
            pat = score;
        }
    }

    return pat;
}

Score search(Board& board, int depth, int ply, Score alpha, Score beta, PVLine* prevLine)
{
    numNodes++;

    if (isDone)
        return 0;

    if (ply == 0 || depth >= 3)
    {
        unsigned long long time = getTime();

        if (time - startTime > maxTime)
        {
            isDone = true;
            return 0;
        }

        if (numNodes > maxNodes)
        {
            isDone = true;
            return 0;
        }
    }

    if (board.getRepetition() == 3)
    {
        return 0; // draw by repetition
    }
    if (board.getState()->move50rule == 50)
        return 0; // 50 move

    bool replaceTT = true;
    TranspositionEntry* entry = tTable->GetEntry(board.getHash());

    Move bestEntryMove = 0;

    if (entry)
    {
        if (entry->depth >= depth)
        {
            if (entry->nodeType == NodeType::Exact)
            {
                return entry->score;
            }
            if (entry->nodeType == NodeType::Upper && entry->score <= alpha)
            {
                return entry->score;
            }
            if (entry->nodeType == NodeType::Lower && entry->score >= beta)
            {
                return entry->score;
            }
            replaceTT = false;
        }

        if (entry->age < (int)board.getState()->ply - 25)
            replaceTT = true;

        bestEntryMove = entry->move;
    }

    if (depth == 0)
    {
        return qsearch(board, ply, alpha, beta);
    }

    prevLine->len = 0;

    MoveList moves;
    generateMoves<ALL_MOVES>(board, &moves);

    if (moves.size == 0)
    {
        if (board.getNumChecks()) // if in check, then checkmate
            return -MATE + ply;
        else // else, stalemate
            return 0;
    }

    if (bestEntryMove.getMove())
        SortMoves(board, &moves, bestEntryMove);
    else
        SortMoves(board, &moves);

    PVLine line;

    Score bestS = -INF;
    Move bestM = 0;

    NodeType nodeType = NodeType::Upper;

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];

        // Late move reductions (LMR)

        int newDepth = depth - 1;
        // if (depth >= LMR_DEPTH && i > LMR_INDEX && move.type() == QUIET && !board.getNumChecks() &&
        // !board.isCheckMove(move))
        //     newDepth--;

        board.makeMove(move);

        Key childHash = board.getHash();

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

        newDepth = std::max(newDepth, 0);

        Score score = -search(board, newDepth, ply + 1, -beta, -alpha, &line);
        board.undoMove();

        if (isDone)
            return 0;

        if (score >= beta)
        {
            prevLine->moves[0] = move;
            prevLine->len = 1;

            tTable->SetEntry(board.getHash(), score, newDepth, NodeType::Lower, board.getState()->ply, move);

            return score;
        }
        if (score > alpha)
        {
            nodeType = NodeType::Exact;
            alpha = score;
        }
        if (score > bestS)
        {
            bestS = score;
            bestM = move;
            prevLine->moves[0] = move;
            memcpy(prevLine->moves + 1, &line.moves, line.len * sizeof(Move));
            prevLine->len = line.len + 1;
        }
    }

    if (replaceTT)
        tTable->SetEntry(board.getHash(), bestS, depth, nodeType, board.getState()->ply, bestM);

    return bestS;
}

Score iterativeDeepening(Board& board, unsigned int depth, unsigned int nodes, unsigned int movetime)
{
    startTime = getTime();

    PVLine pv;
    pv.len = 0;

    Move prevBestMove = 0;
    Score prevBestScore = -INF;

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
                          << " nodes " << numNodes << " pv " << GetMoveListString(&pv) << std::endl;
            }
        }

        std::cout << "info depth " << d << " curmov " << bestMove.toString() << " score cp " << bestScore << " nodes "
                  << numNodes << " pv " << GetMoveListString(&pv) << "hashfull " << (int)(tTable->GetFull() * 1000)
                  << " time " << getTime() - startTime << std::endl;

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