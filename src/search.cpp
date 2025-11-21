#include <cmath>
#include <cstring>
#include <iostream>

#include "search.h"

#include "MoveSort.h"
#include "evaluate.h"
#include "profile.h"
#include "time.h"
#include "transposition.h"

#define INF 1000000
#define MATE 100000

#define MAX_DEPTH 256
#define FUTILITY_DEPTH 3
#define LMR_INDEX 2
#define LMR_DEPTH 3
#define NULL_DEPTH 4
#define IIR_DEPTH 4 // internal iterative reduction depth
#define FUTILITY_MARGIN(DEPTH) 50 + 100 * DEPTH
#define DELTA 200

unsigned long long numNodes;
unsigned long long numQNodes;
unsigned long long numBetaCutoffs;
unsigned long long ttHits;
unsigned long long maxNodes;
unsigned long long maxTime;
unsigned long long startTime;
static TranspositionTable* tTable =
    new (std::align_val_t(64)) TranspositionTable(1024 * 1024 * 64); // transposition table with a size of 64 MB

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
    PROFILE_FUNC();
    numQNodes++;

    // probe tt

    TranspositionEntry* entry = tTable->GetEntry(board.getHash());
    Move bestEntryMove = 0;
    if (entry && entry->depth >= 0)
    {
        if (entry->bound == NodeBound::Exact)
        {
            return entry->score;
        }
        if (entry->bound == NodeBound::Upper && entry->score <= alpha)
        {
            return entry->score;
        }
        if (entry->bound == NodeBound::Lower && entry->score >= beta)
        {
            return entry->score;
        }

        bestEntryMove = entry->move;
    }

    Score pat = Eval(board);

    if (pat >= beta)
        return pat;

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

    SortMovesQ(board, &moves, bestEntryMove);

    PVLine line;
    Move bestM = 0;

    for (int i = 0; i < moves.size; i++)
    {
        Move m = moves.moves[i];

        // Delta pruning
        if (pat + pieceScores[getType(m.captured())] < alpha - DELTA)
            continue;

        board.makeMove(m);
        Score score = -qsearch(board, ply + 1, -beta, -alpha);
        board.undoMove();

        if (score > pat)
        {
            pat = score;
            bestM = m;

            if (score > alpha)
            {
                if (score >= beta)
                {
                    tTable->SetEntry(board.getHash(), score, 0, NodeBound::Lower, board.getPly() - ply, m);
                    return score;
                }
                alpha = score;
            }
        }
    }

    tTable->SetEntry(board.getHash(), pat, 0, NodeBound::Upper, board.getPly() - ply, bestM);

    return pat;
}

template <NodeType node>
Score search(Board& board, int depth, int ply, Score alpha, Score beta, PVLine* prevLine)
{
    if (isDone)
        return 0;

    if (getTime() - startTime > maxTime)
    {
        isDone = true;
        return 0;
    }

    if (numNodes > maxNodes)
    {
        isDone = true;
        return 0;
    }
    numNodes++;

    if (board.getRepetition() == 3)
    {
        return 0; // draw by repetition
    }
    if (board.getState()->move50rule == 100)
        return 0; // 50 fullmoves have been made

    constexpr bool isPVNode = node == PVNode;

    TranspositionEntry* entry = tTable->GetEntry(board.getHash());

    Move bestEntryMove = 0;

    if (entry)
    {
        ttHits++;
        if (entry->depth >= depth)
        {
            if (entry->bound == NodeBound::Exact)
            {
                return entry->score;
            }
            if (entry->bound == NodeBound::Upper && entry->score <= alpha)
            {
                return entry->score;
            }
            if (entry->bound == NodeBound::Lower && entry->score >= beta)
            {
                return entry->score;
            }
        }

        bestEntryMove = entry->move;
    }
    else if (depth > IIR_DEPTH)
    {
        // Internal Iterative Reduction if no hashmove found (reduce depth by one)
        depth--;
    }

    if (depth == 0)
    {
        Score q = qsearch(board, ply, alpha, beta);
        return q;
    }

    PVLine line;

    MoveList moves;
    generateMoves<ALL_MOVES>(board, &moves); // compute moves here because generateMoves computes checks

    if (moves.size == 0)
    {
        if (board.getNumChecks()) // if in check, then checkmate
            return -MATE + ply;
        else // else, stalemate
            return 0;
    }

    // null move pruning
    int numEnemyPieces = popCount(board.getBB(ALL_PIECES, ~board.sideToMove) & ~board.getBB(PAWN));
    if (numEnemyPieces > 0 && board.getNumChecks() == 0 && depth >= NULL_DEPTH + 1)
    {
        board.makeNullMove();
        Score nullScore = -search<CUTNode>(board, depth - NULL_DEPTH, ply + 1, -beta, -beta + 1, &line);
        board.undoNullMove();
        if (nullScore > beta)
            return nullScore;
        line.len = 0;
    }

    prevLine->len = 0;

    if (bestEntryMove.getMove())
        SortMoves(board, &moves, bestEntryMove);
    else
        SortMoves(board, &moves);

    Score bestS = -INF;
    Move bestM = 0;

    NodeBound nodeBound = NodeBound::Upper;

    for (int i = 0; i < moves.size; i++)
    {
        Move move = moves.moves[i];

        bool fullSearch = true;
        int extension = 0;
        bool checkMove = board.isCheckMove(move);

        if (checkMove)
            extension++;

        // Futility pruning
        if (depth < FUTILITY_DEPTH && move.type() == QUIET && !checkMove)
        {
            Score eval = Eval(board) + FUTILITY_MARGIN(depth);
            if (eval <= alpha)
                continue;
        }

        board.makeMove(move);

        // Late move reductions (LMR)
        Score score;
        if (isPVNode && depth >= LMR_DEPTH && i >= LMR_INDEX && move.type() == QUIET && extension == 0)
        {
            int reduction = depth / 3;

            score = -search<CUTNode>(board, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha, &line);
            fullSearch = score > alpha; // if score is better than we thought, research at full depth
        }

        if (fullSearch)
        {
            // Depth Extensions/reductions
            score = -search<node>(board, depth + extension - 1, ply + 1, -beta, -alpha, &line);
        }
        board.undoMove();

        if (isDone)
            return 0;

        if (!fullSearch) // if we didn't do a full search, don't process score
            continue;

        if (score >= beta)
        {
            prevLine->moves[0] = move;
            prevLine->len = 1;

            if (move.isType<QUIET>())
            {
                addKillerMove(board.getPly(), move);
                addHistoryBonus(!board.whiteToMove, move, depth); // add move history bonus

                for (int p = 0; p < moves.size; p++)
                {
                    Move penaltyMove = moves.moves[p];

                    if (penaltyMove.isType<QUIET>() && penaltyMove != move)
                        addHistoryPenalty(!board.whiteToMove, penaltyMove, depth);
                }
            }

            if (!isDone) // don't save on incomplete search
                tTable->SetEntry(board.getHash(), score, depth, NodeBound::Lower, board.getPly() - ply, move);
            numBetaCutoffs++;
            return score;
        }
        if (score > alpha)
        {
            nodeBound = NodeBound::Exact;
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

    if (bestM == 0)         // if we didn't search a move (futility pruned all moves)
        return Eval(board); // return static evaluation

    if (!isDone) // don't store in transposition table because we cutoff early (Time cutoff, node cutoff, etc.)
        tTable->SetEntry(board.getHash(), bestS, depth, nodeBound, board.getPly() - ply, bestM);

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
    numQNodes = 0;
    numBetaCutoffs = 0;
    ttHits = 0;

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
            Score eval = -search<PVNode>(board, d - 1, 0, bestScore, INF, &line);
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
                          << " nodes " << numNodes + numQNodes << " TTHitRate " << GetMoveListString(&pv) << std::endl;
            }
        }

        std::cout << "info depth " << d << " curmov " << bestMove.toString() << " score cp " << bestScore << " nodes "
                  << numNodes + numQNodes << " pv " << GetMoveListString(&pv) << "hashfull "
                  << (int)(tTable->GetFull() * 1000) << " time " << getTime() - startTime << " TTHitRate "
                  << (float)ttHits / (float)numNodes << " BetaCutRate " << (float)numBetaCutoffs / (float)numNodes
                  << " pv " << std::endl;

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