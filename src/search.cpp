#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>

#include "search.h"

#include "MoveSort.h"
#include "evaluate.h"
#include "profile.h"
#include "time.h"
#include "transposition.h"

#define INF 32000
#define MATE 31000

#define MAX_DEPTH 256
#define FUTILITY_DEPTH 4

constexpr int lmr_index = 2; // the first index lmr will be used on
constexpr int lmr_depth = 2; // the minimum depth lmr can be used

constexpr Score aspirationStartingDelta = 30;
constexpr float aspirationMultiplier = 1.5f;

#define NULL_DEPTH 3
#define IIR_DEPTH 6 // internal iterative reduction depth
#define FUTILITY_MARGIN(DEPTH) 80 + 120 * DEPTH
#define DELTA 200

unsigned long long numNodes;
unsigned long long numQNodes;
unsigned long long numBetaCutoffs;
unsigned long long ttHits;
unsigned long long pvHits;
unsigned long long orderingNodes;
unsigned long long maxNodes;
unsigned long long maxTime;
unsigned long long startTime;

bool isDone;

constexpr auto logTable = [] {
    std::array<float, 256> table{};
    table[0] = 0;
    for (int i = 1; i < 256; i++)
        table[i] = std::log(i);

    return table;
}();

struct PVLine
{
    unsigned int len;
    Move moves[MAX_DEPTH];

    PVLine() : len(0)
    {
    }
};

Score bestScore = -INF;
Move bestMove = 0;
Move prevBestMove = 0;

inline bool isWin(Score s)
{
    return s > MATE - MAX_DEPTH;
}

inline bool isLoss(Score s)
{
    return s < -MATE + MAX_DEPTH;
}

inline Score mateToTT(Score s, unsigned char ply)
{
    return isWin(s) ? s + ply : (isLoss(s) ? s - ply : s);
}

inline Score ttToMate(Score s, unsigned char ply)
{
    return isWin(s) ? s - ply : (isLoss(s) ? s + ply : s);
}

void UpdatePV(PVLine* line, Move move, PVLine* prev)
{
    prev->moves[0] = move;
    for (unsigned int i = 0; i < line->len; i++)
    {
        prev->moves[i + 1] = line->moves[i];
    }

    prev->len = line->len + 1;
}

/**
 * @brief Calculates the depth reduction for LMR
 *
 * @param depth the current depth
 * @param moveNum the current move number
 * @return constexpr int
 */
constexpr int LMRReduction(int depth, int moveNum)
{
    return 0.75f + logTable[depth] * logTable[moveNum] / 2.25f;
}

std::string GetMoveListString(PVLine* l)
{
    std::string moves;
    for (unsigned int i = 0; i < l->len; i++)
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

    if (board.getState()->repetition == 3)
        return 0; // draw by repetition
    if (board.getState()->move50rule == 100)
        return 0; // 50-move draw

    TranspositionEntry* entry = tTable->GetEntry(board.getHash());
    Move bestEntryMove = 0;
    if (entry)
    {
        Score corrected = ttToMate(entry->score, ply);
        if (entry->getNodeBound() == NodeBound::Exact ||
            (entry->getNodeBound() == NodeBound::Upper && corrected <= alpha) ||
            (entry->getNodeBound() == NodeBound::Lower && corrected >= beta))
        {
            entry->setAge(tTable->GetAge()); // reset the age for this node
            return corrected;
        }

        bestEntryMove = entry->move;
    }

    Score pat = Eval<FULL>(board);

    if (pat >= beta)
        return pat;

    if (alpha < pat)
        alpha = pat;

    // Generate moves

    MoveList moves;
    if (!board.getNumChecks()) // If not in check, generate captures
    {
        board.generateMoves<CAPTURE>(&moves);
        if (!moves.size) // if no moves, check for stalemate
        {
            board.generateMoves<ALL_MOVES>(&moves);
            if (moves.size == 0)
                return 0; // stalemate
            return pat;
        }
    }
    else // if in check, generate evasions
    {
        board.generateMoves<ALL_MOVES>(&moves);
        if (!moves.size) // if no moves, checkmate
        {
            return -MATE + ply;
        }
    }

    MoveSorter sorter(board, &moves, bestEntryMove);

    PVLine line;
    Move bestM = 0;
    BoardState state;

    for (; sorter.size != 0;)
    {
        Move m = sorter.Next();

        Piece movePiece = board.getSQ(m.to());

        // Delta pruning
        if (!board.getNumChecks())
        {
            if (pat + pieceScores[getType(movePiece)] < alpha - DELTA)
                continue;
        }

        board.makeMove(m, &state);
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
                    tTable->SetEntry(board.getHash(), mateToTT(score, ply), 0, NodeBound::Lower, m);
                    numBetaCutoffs++;
                    return score;
                }
                alpha = score;
            }
        }
    }
    if (bestM.getMove() != 0)
        tTable->SetEntry(board.getHash(), mateToTT(pat, ply), 0, NodeBound::Upper, bestM);

    return pat;
}

template <NodeType node>
Score search(Board& board, int depth, int ply, Score alpha, Score beta, PVLine* prevLine)
{
    if (isDone)
        return 0;

    if ((numNodes & 2047) == 0 && getTime() - startTime > maxTime)
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

    prevLine->len = 0;

    constexpr bool isPVNode = node == PVNode || node == RootNode;
    constexpr bool isRootNode = node == RootNode;

    if (!isRootNode && board.getState()->repetition == 3)
        return 0; // draw by repetition

    if (!isRootNode && board.getState()->move50rule == 100)
        return 0; // 50 fullmoves have been made

    Move bestEntryMove = 0;

    TranspositionEntry* entry = tTable->GetEntry(board.getHash());

    if (entry)
    {
        ttHits++;
        if (entry->depth >= depth)
        {
            Score corrected = ttToMate(entry->score, ply);
            if (!isPVNode && (entry->getNodeBound() == NodeBound::Exact ||
                              (entry->getNodeBound() == NodeBound::Upper && corrected <= alpha) ||
                              (entry->getNodeBound() == NodeBound::Lower && corrected >= beta)))
            {
                entry->setAge(tTable->GetAge()); // reset the age for this node
                return corrected;
            }
            else if (isPVNode)
            {
                if (entry->getNodeBound() == NodeBound::Lower)
                    alpha = std::max(alpha, corrected);
            }
        }

        bestEntryMove = entry->move;
    }
    else if (!isPVNode && depth > IIR_DEPTH)
    {
        // Internal Iterative Reduction if no hashmove found (reduce depth by one)
        depth--;
    }

    if (depth == 0)
    {
        Score q = qsearch(board, ply, alpha, beta);
        return q;
    }

    MoveList moves;
    board.generateMoves<ALL_MOVES>(&moves); // compute moves here because generateMoves computes checks

    if (moves.size == 0)
    {
        if (board.getNumChecks()) // if in check, then checkmate
            return -MATE + ply;
        else // else, stalemate
            return 0;
    }

    // reverse futility pruning
    if (!isPVNode && !board.getNumChecks() && depth <= 8)
    {
        Score eval = Eval<FAST>(board);
        Score margin = 120 * depth;

        if (eval - margin >= beta)
        {
            return eval;
        }
    }

    // razoring
    if (!isPVNode && !board.getNumChecks() && depth <= 3)
    {
        Score eval = Eval<FAST>(board);
        Score margin = 300 + (100 * depth);

        if (eval + margin <= alpha)
        {
            return qsearch(board, ply, alpha, beta);
        }
    }

    BoardState state;

    // null move pruning

    int numEnemyPieces = popCount(board.getBB(ALL_PIECES, ~board.sideToMove) & ~board.getBB(PAWN));
    if (!isPVNode && numEnemyPieces > 0 && board.getNumChecks() == 0 && depth >= NULL_DEPTH && !isLoss(beta))
    {
        PVLine line;

        board.makeNullMove(&state);
        Score nullScore = -search<CUTNode>(board, depth * 2 / 3 - 1, ply + 1, -beta, -beta + 1, &line);
        board.undoNullMove();
        if (nullScore >= beta)
        {
            return nullScore;
        }
    }

    if (node == RootNode)
        bestEntryMove = prevBestMove;

    MoveSorter sorter(board, &moves, bestEntryMove);

    Score bestS = -INF;
    Move bestM = 0;

    NodeBound nodeBound = NodeBound::Upper;
    Move firstMove;
    for (int i = 0; sorter.size != 0; i++)
    {
        PVLine line;

        Move move = sorter.Next();
        int extension = 0;
        bool checkMove = board.isCheckMove(move);

        if (checkMove)
            extension = 1;
        // if (isPVNode && move.to() == board.getState()->move.to()) // recapture extension
        //     extension = 1;

        // Futility pruning
        if (!isPVNode && depth < FUTILITY_DEPTH && move.type() == QUIET && !checkMove)
        {
            Score eval = Eval<FAST>(board) + FUTILITY_MARGIN(depth);
            if (eval <= alpha)
                continue;
        }

        board.makeMove(move, &state);

        // Late move reductions (LMR)
        Score score;
        bool fullSearch = i == 0; // always full search the first move
        if (fullSearch)
            firstMove = move;

        if (!fullSearch) // pvs
        {
            int reductions = 0;
            if (depth >= lmr_depth && i >= lmr_index && !checkMove && move.isType<QUIET>()) // lmr
                reductions = LMRReduction(depth, i);

            score = -search<CUTNode>(board, depth - reductions - 1, ply + 1, -alpha - 1, -alpha, &line);

            fullSearch = score > alpha;
            line.len = 0;
        }

        if (fullSearch)
        {
            // Depth Extensions/reductions
            if constexpr (node == RootNode)
                score = -search<PVNode>(board, depth + extension - 1, ply + 1, -beta, -alpha, &line);
            else
                score = -search<node>(board, depth + extension - 1, ply + 1, -beta, -alpha, &line);
        }
        board.undoMove();

        if (isDone)
            return 0;

        if (score >= beta)
        {
            if (isDone) // don't save on incomplete search
                return score;

            if (isPVNode)
            {
                prevLine->moves[0] = move;
                prevLine->len = 1;
                if (line.len > 0)
                {
                    UpdatePV(&line, move, prevLine);
                }
            }

            updateContinuationHistory(board, move, depth, false);

            if (move.isType<QUIET>())
            {
                counterMove[board.getState()->move.from()][board.getState()->move.to()] = move;

                addKillerMove(board.getPly(), move);
                addHistoryBonus(!board.whiteToMove, move, depth); // add move history bonus

                for (int p = moves.size - i; p < moves.size; p++)
                {
                    Move penaltyMove = sorter.moveVals[p].m;

                    if (penaltyMove == move)
                        continue;
                    if (penaltyMove.isType<QUIET>())
                        addHistoryPenalty(!board.whiteToMove, penaltyMove, depth);
                    updateContinuationHistory(board, penaltyMove, depth, true);
                }
            }
            else if (move.isType<CAPTURE>())
            {
                PieceType victimType = getType(board.getSQ(move.to()));
                if (move.to() == board.getEnPassantSqr())
                    victimType = PAWN;
                addCaptureBonus(victimType, move, depth); // add move history bonus

                for (int p = moves.size - i; p < moves.size; p++)
                {
                    Move penaltyMove = sorter.moveVals[p].m;
                    if (penaltyMove == move)
                        continue;
                    if (penaltyMove.isType<CAPTURE>())
                    {
                        victimType = getType(board.getSQ(penaltyMove.to()));
                        if (penaltyMove.to() == board.getEnPassantSqr())
                            victimType = PAWN;
                        addCapturePenalty(victimType, penaltyMove, depth);
                    }
                    updateContinuationHistory(board, penaltyMove, depth, true);
                }
            }

            tTable->SetEntry(board.getHash(), mateToTT(score, ply), depth, NodeBound::Lower, move);
            numBetaCutoffs++;
            return score;
        }
        if (score > bestS)
        {
            if (score > alpha)
            {
                nodeBound = NodeBound::Exact;
                alpha = score;

                if (isPVNode)
                {
                    UpdatePV(&line, move, prevLine);
                }
            }
            bestS = score;
            bestM = move;

            if (node == RootNode)
            {
                bestMove = move;
                bestScore = score;
                std::cout << "info depth " << depth << " curmov " << bestMove.toString() << " score cp " << bestScore
                          << " nodes " << numNodes + numQNodes << " pv " << GetMoveListString(prevLine) << std::endl;
            }
        }
    }

    if (firstMove == bestM)
        pvHits++;

    if (moves.size >= 2)
        orderingNodes++;

    if (bestM.getMove() == 0)     // if we didn't search a move (futility pruned all moves)
        return Eval<FAST>(board); // return static evaluation

    if (!isDone) // don't store in transposition table because we cutoff early (Time cutoff, node cutoff, etc.)
        tTable->SetEntry(board.getHash(), mateToTT(bestS, ply), depth, nodeBound, bestM);

    return bestS;
}

Score iterativeDeepening(Board& board, unsigned int depth, unsigned int nodes, unsigned int movetime)
{
    startTime = getTime();

    PVLine pv;
    pv.len = 0;

    Score prevBestScore = 0;
    prevBestMove = 0;

    bestMove = 0;
    bestScore = -INF;
    numNodes = 0;
    numQNodes = 0;
    numBetaCutoffs = 0;
    ttHits = 0;
    pvHits = 0;
    orderingNodes = 0;

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
        bestMove = 0;
        bestScore = -INF;

        Score delta = aspirationStartingDelta;
        Score alpha = prevBestScore - delta;
        Score beta = prevBestScore + delta;

        Score eval;
        while (true)
        {
            if (isDone)
                break;

            eval = search<RootNode>(board, d, 0, alpha, beta, &pv);

            delta *= aspirationMultiplier;
            if (eval > alpha && eval < beta)
                break;
            else if (eval <= alpha)
            {
                beta = alpha + 1;
                alpha = std::max(alpha - delta, -INF);
            }
            else
            {
                beta += delta;
            }
        }

        if (isDone)
        {
            bestMove = prevBestMove;
            bestScore = prevBestScore;
            return prevBestScore;
        }

        std::cout << "info depth " << d << " curmov " << bestMove.toString() << " score cp " << eval << " nodes "
                  << numNodes + numQNodes << " pv " << GetMoveListString(&pv) << "hashfull "
                  << (int)(tTable->GetFull() * 1000) << " time " << getTime() - startTime << " TTHitRate "
                  << (float)ttHits / (float)numNodes << " BetaCutRate " << (float)numBetaCutoffs / (float)numNodes
                  << " PVHitRate " << (float)pvHits / (float)orderingNodes << std::endl;

        prevBestMove = bestMove;
        prevBestScore = bestScore;
    }

    return bestScore;
}

Move startSearch(Board& board, unsigned int depth, unsigned int nodes, unsigned int movetime,
                 unsigned int remaining_time)
{
    isDone = false;

    if (remaining_time > 0)
    {
        float maxTime = (float)remaining_time / 25.0f;

        MoveList mlist;
        board.generateMoves<ALL_MOVES>(&mlist);
        if (mlist.size > 30)
            maxTime *= 1.5;
        else if (mlist.size < 10)
            maxTime *= 0.75;
        else
            maxTime *= 0.9;

        movetime = (unsigned int)std::min(maxTime, (float)remaining_time * 0.2f); // use at most 20% of remaining time
        if (movetime == 0)
            movetime = 1;
    }

    tTable->IncrementAge();
    Score eval = iterativeDeepening(board, depth, nodes, movetime);

    std::cout << "bestmove " << bestMove.toString() << " score cp " << eval << std::endl;

    return bestMove;
}