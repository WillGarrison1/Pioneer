#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#include <algorithm>

#define CAPTURE_BONUS 10000
#define PROMOTION_BONUS 9000

#define MVV_LVA_VICTIM_MULTI 3
#define MVV_LVA_ATTACKER_MULTI_GOOD 5
#define MVV_LVA_ATTACKER_MULTI_BAD 1

#define DEFENDED_BONUS 25
#define ATTACKED_PENALTY -35

#define PV_BONUS 30000
#define KILLER_MOVE_BONUS 8000
#define COUNTERMOVE_BONUS 4000
#define MAX_HISTORY 1000

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

MoveVal ScoreMove(const Board &board, Move m);
MoveVal ScoreMoveQ(const Board &board, Move m);

template <SortType S>
struct MoveSorter
{
    MoveVal moveVals[256];
    unsigned int size;

    MoveSorter(const Board &board, MoveList *mlist, Move best) : size(mlist->size)
    {
        for (int i = 0; i < size; i++)
        {
            if (mlist->moves[i] == best)
                moveVals[i] = {best, PV_BONUS};
            else
            {
                if constexpr (S == QUIESCENCE)
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

        for (int i = 0; i < size; i++)
        {
            if (moveVals[i].score > bestScore)
            {
                bestScore = moveVals[i].score;
                bestIndex = i;
            }
        }

        Move bestMove = moveVals[bestIndex].m;

        // Remove the best move from the list
        moveVals[bestIndex] = moveVals[--size];

        return bestMove;
    }
};

extern Move killerMoves[MAX_PLY][2]; // each ply can have two killer moves
extern int moveHistory[2][64][64];   // History for [isBlack][from][to]
extern Move counterMove[64][64];

inline bool isKillerMove(unsigned char ply, Move m)
{
    return killerMoves[ply][0] == m || killerMoves[ply][1] == m;
}

inline void addKillerMove(unsigned char ply, Move m)
{
    if (isKillerMove(ply, m))
        return;

    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = m;
}

inline void addHistoryBonus(bool isBlack, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isBlack][m.from()][m.to()] +=
        clampedBonus - moveHistory[isBlack][m.from()][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;
}

inline void addHistoryPenalty(bool isBlack, Move m, int depth)
{

    const int penalty = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isBlack][m.from()][m.to()] -=
        penalty + moveHistory[isBlack][m.from()][m.to()] * std::abs(penalty) / MAX_HISTORY;
}

inline Score Mvv_Lva_Score(const Board &board, Move m)
{
    Bitboard attacked = board.getAttacked(~board.sideToMove);
    bool isVictimDefended = (sqrToBB(m.to()) & attacked) != 0;
    PieceType victimType = getType(board.getSQ(m.to()));
    PieceType pieceType = getType(board.getSQ(m.from()));

    return (pieceScores[victimType] * MVV_LVA_VICTIM_MULTI - pieceScores[pieceType]) *
           (isVictimDefended ? MVV_LVA_ATTACKER_MULTI_BAD : MVV_LVA_ATTACKER_MULTI_GOOD);
}
#endif