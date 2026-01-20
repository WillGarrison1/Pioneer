#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#include <algorithm>

#define CAPTURE_BONUS 400000
#define PROMOTION_BONUS 500000

#define MVV_LVA_VICTIM_MULTI 4
#define MVV_LVA_ATTACKER_MULTI_GOOD 5
#define MVV_LVA_ATTACKER_MULTI_BAD 1

#define DEFENDED_BONUS 100
#define ATTACKED_PENALTY -100

#define PV_BONUS 1000000
#define KILLER_MOVE_BONUS 300000
#define COUNTERMOVE_BONUS 200000
#define MAX_HISTORY 20000
#define MAX_CAPTURE_HISTORY 30000

#define CONTINUATION_HISTORY_SIZE 3

struct MoveVal
{
    Move m;
    int score;
};

enum SortType
{
    NORMAL,
    QUIESCENCE
};

MoveVal ScoreMove(const Board& board, Move m);
MoveVal ScoreMoveQ(const Board& board, Move m);

struct MoveSorter
{
    MoveVal moveVals[256];
    unsigned int size;

    MoveSorter(const Board& board, MoveList* mlist, Move best) : size(mlist->size)
    {
        for (unsigned int i = 0; i < size; i++)
        {
            if (mlist->moves[i] == best)
                moveVals[i] = {best, PV_BONUS};
            else
            {
                if (mlist->moves[i].isType<CAPTURE>())
                    moveVals[i] = ScoreMoveQ(board, mlist->moves[i]);
                else
                    moveVals[i] = ScoreMove(board, mlist->moves[i]);
            }
        }
    }

    ~MoveSorter() = default;
    Move Next()
    {
        int bestIndex = 0;
        int bestScore = -1000000;

        for (unsigned int i = 0; i < size; i++)
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

extern Move killerMoves[MAX_PLY][2];                // each ply can have two killer moves
extern int moveHistory[2][64][64];                  // History for [isBlack][from][to]
extern int captureHistory[64][64][PieceType::KING]; // same as moveHistory
extern Move counterMove[64][64];
extern int continuationHistory[CONTINUATION_HISTORY_SIZE][6][64][6][64];

inline void addKillerMove(unsigned char ply, Move m)
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

inline void addHistoryBonus(bool isBlack, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth * depth, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isBlack][m.from()][m.to()] +=
        clampedBonus - moveHistory[isBlack][m.from()][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;
}

inline void addHistoryPenalty(bool isBlack, Move m, int depth)
{

    const int penalty = std::clamp(depth * depth * depth, -MAX_HISTORY, MAX_HISTORY);
    auto gravity = moveHistory[isBlack][m.from()][m.to()] * std::abs(penalty) / MAX_HISTORY;
    moveHistory[isBlack][m.from()][m.to()] -= penalty + gravity;
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

inline Score Mvv_Lva_Score(const Board& board, Move m)
{
    PieceType victimType = getType(board.getSQ(m.to()));
    PieceType pieceType = getType(board.getSQ(m.from()));

    return (pieceScores[victimType] - pieceScores[pieceType]);
}
#endif