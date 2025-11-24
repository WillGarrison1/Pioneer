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

    MoveVal() = default;

    MoveVal(Move m, int score) : m(m), score(score)
    {
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

extern MoveVal ScoreMove(const Board &board, Move m);

extern MoveVal ScoreMoveQ(const Board &board, Move m);

/**
 * @brief Sorts moves for qsearch
 *
 * @param board
 * @param moves
 */
inline void SortMovesQ(const Board &board, MoveList *moves);

extern void SortMovesQ(const Board &board, MoveList *moves, Move best);

extern void SortMoves(const Board &board, MoveList *moves);

extern void SortMoves(const Board &board, MoveList *moves, Move prev);
#endif