#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#if TUNING
#include "tuning_params.h"
#else
#include "searchParams.h"
#endif

#include <algorithm>

#define CONTINUATION_HISTORY_SIZE 3

struct MoveVal
{
    Move m;
    int16_t score;
};

enum SortType
{
    NORMAL,
    QUIESCENCE
};

MoveVal ScoreMove(const Board& board, Move m, int ply);
MoveVal ScoreMoveQ(const Board& board, Move m);

struct MoveSorter
{
    MoveVal moveVals[256];
    unsigned int size;

    MoveSorter(const Board& board, MoveList* mlist, Move best, int ply) : size(0)
    {

        while (size < mlist->GetSize())
        {
            if (mlist->moves[size] == best)
            {
                moveVals[size] = moveVals[0];
                moveVals[0] = {best, PV_BONUS};
            }
            else
            {
                if (mlist->moves[size].isType<CAPTURE>())
                    moveVals[size] = ScoreMoveQ(board, mlist->moves[size]);
                else
                    moveVals[size] = ScoreMove(board, mlist->moves[size], ply);
            }
            size++;
        }
    }

    ~MoveSorter() = default;
    Move Next()
    {
        int bestIndex = 0;
        int bestScore = moveVals[0].score;

        if (bestScore != PV_BONUS) // if the best move is the PV move, return it immediately
            for (unsigned int i = 1; i < size; i++)
            {
                if (moveVals[i].score > bestScore)
                {
                    bestScore = moveVals[i].score;
                    bestIndex = i;
                }
            }

        MoveVal bestMove = moveVals[bestIndex];

        // Swap the best move with the end list
        moveVals[bestIndex] = moveVals[--size];
        moveVals[size] = bestMove;

        return bestMove.m;
    }
};

extern Move killerMoves[MAX_PLY][2];                    // each ply can have two killer moves
extern int16_t moveHistory[2][64][64];                  // History for [isWhite][from][to]
extern int16_t captureHistory[64][64][PieceType::KING]; // same as moveHistory
extern Move counterMove[64][64];
extern int16_t continuationHistory[CONTINUATION_HISTORY_SIZE][6][64][6][64];

// `ply` is the SEARCH ply, not board.getPly(). It used to be the latter, narrowed to
// `unsigned char` on the way in while ScoreMove read the table with the full-width board ply --
// so past ply 255 the write and read indices diverged and killers silently stopped working.
inline void addKillerMove(int ply, Move m)
{
    if (killerMoves[ply][0] == m)
        return;

    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = m;
}

inline void updateContinuationHistory(Board& board, Move m, int depth, bool negate)
{
    int negative = negate ? -1 : 1;
    const BoardState* prevState = board.getState();
    PieceType moved = getType(board.getSQ(m.from()));
    int clampedBonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY) * negative;
    for (int i = 0; i < CONTINUATION_HISTORY_SIZE; i++)
    {
        if (!prevState || prevState->moved == EMPTY)
            break;

        PieceType pType = getType(prevState->moved);
        Move prevMove = prevState->move;
        continuationHistory[i][pType - 1][prevMove.to()][moved - 1][m.to()] +=
            clampedBonus -
            continuationHistory[i][pType - 1][prevMove.to()][moved - 1][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;

        prevState = prevState->prev;
    }
}

inline void addHistoryBonus(bool isWhite, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth * 50, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isWhite][m.from()][m.to()] +=
        clampedBonus - moveHistory[isWhite][m.from()][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;
}

inline void addHistoryPenalty(bool isWhite, Move m, int depth)
{
    const int penalty = std::clamp(depth * depth * 50, -MAX_HISTORY, MAX_HISTORY);
    auto gravity = moveHistory[isWhite][m.from()][m.to()] * std::abs(penalty) / MAX_HISTORY;
    moveHistory[isWhite][m.from()][m.to()] -= penalty + gravity;
}

inline void addCaptureBonus(PieceType victimType, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth * depth, -MAX_CAPTURE_HISTORY, MAX_CAPTURE_HISTORY);
    captureHistory[m.from()][m.to()][victimType - 1] +=
        clampedBonus - captureHistory[m.from()][m.to()][victimType - 1] * std::abs(clampedBonus) / MAX_CAPTURE_HISTORY;
}

inline void addCapturePenalty(PieceType victimType, Move m, int depth)
{
    const int penalty = std::clamp(depth * depth * depth, -MAX_CAPTURE_HISTORY, MAX_CAPTURE_HISTORY);
    captureHistory[m.from()][m.to()][victimType - 1] -=
        penalty + captureHistory[m.from()][m.to()][victimType - 1] * std::abs(penalty) / MAX_CAPTURE_HISTORY;
}

/**
 * @brief The piece type captured by a CAPTURE-typed move.
 *
 * En passant is encoded as a plain CAPTURE onto an empty square, so an empty destination
 * identifies it unambiguously. Testing `m.to() == board.getEnPassantSqr()` instead is wrong:
 * that is true for *any* piece landing on the en passant square, not just a pawn capturing there.
 */
inline PieceType CapturedType(const Board& board, Move m)
{
    const Piece victim = board.getSQ(m.to());
    return victim == EMPTY ? PAWN : getType(victim);
}

inline Score Mvv_Lva_Score(const Board& board, Move m)
{
    PieceType victimType = getType(board.getSQ(m.to()));
    PieceType pieceType = getType(board.getSQ(m.from()));

    return (pieceScores[victimType] - pieceScores[pieceType]);
}
#endif