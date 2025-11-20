#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#include <algorithm>

#define CAPTURE_BONUS 4000
#define PROMOTION_BONUS 4000

#define MVV_LVA_VICTIM_MULTI 3
#define MVV_LVA_ATTACKER_MULTI_GOOD 1
#define MVV_LVA_ATTACKER_MULTI_BAD 5

#define DEFENDED_BONUS 25
#define ATTACKED_PENALTY -35

#define PV_BONUS 30000
#define KILLER_MOVE_BONUS 3000
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
    moveHistory[isBlack][m.from()][m.to()] += clampedBonus - moveHistory[isBlack][m.from()][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;
}

inline void addHistoryPenalty(bool isBlack, Move m, int depth)
{

    const int penalty = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isBlack][m.from()][m.to()] -= penalty + moveHistory[isBlack][m.from()][m.to()] * std::abs(penalty) / MAX_HISTORY;
}

inline Score Mvv_Lva_Score(Board &board, Move m)
{
    Bitboard attacked = board.getAttacked(~board.sideToMove);
    bool isVictimDefended = (sqrToBB(m.to()) & attacked) != 0;

    return (pieceScores[getType(m.captured())] * MVV_LVA_VICTIM_MULTI - pieceScores[getType(m.piece())]) *
           (isVictimDefended
                ? MVV_LVA_ATTACKER_MULTI_BAD
                : MVV_LVA_ATTACKER_MULTI_GOOD);
}

inline MoveVal ScoreMove(Board &board, Move m)
{
    const Color us = getColor(m.piece());

    MoveVal v(m, 0);

    if (isKillerMove(board.getPly(), m))
        v.score += KILLER_MOVE_BONUS;

    if (m.type() & CAPTURE) // bonus for captures
    {
        v.score += Mvv_Lva_Score(board, m) + CAPTURE_BONUS;
    }

    if (m.type() & PROMOTION) // bonus for promotions
        v.score += pieceScores[getType(m.promotion())] + PROMOTION_BONUS;

    if (m.isType<QUIET>())
        v.score += moveHistory[board.sideToMove == BLACK][m.from()][m.to()];

    if (sqrToBB(m.to()) & board.getAttacked(~us)) // penalty for moving piece to attacked square
        v.score += (ATTACKED_PENALTY - pieceScores[getType(m.piece())]) >> 1;

    // if (sqrToBB(m.to()) & board.getAttacked(us)) // bonus for moving piece to defended square
    //     v.score += DEFENDED_BONUS + pieceScores[getType(m.piece())] >> 5;

    v.score += us == WHITE
                   ? GetPSQValue<WHITE>(getType(m.piece()), m.to()) - GetPSQValue<WHITE>(getType(m.piece()), m.from())
                   : GetPSQValue<BLACK>(getType(m.piece()), m.to()) - GetPSQValue<BLACK>(getType(m.piece()), m.from());

    return v;
}

inline MoveVal ScoreMoveQ(Board &board, Move m)
{
    MoveVal v(m, 0);

    v.score += Mvv_Lva_Score(board, m);

    if (m.type() & PROMOTION) // bonus for promotions
        v.score += pieceScores[getType(m.promotion())] + PROMOTION_BONUS;

    // if (sqrToBB(m.to()) & board.getAttacked(~us)) // penalty for moving piece to attacked square
    //     v.score += ATTACKED_PENALTY;

    // if (sqrToBB(m.to()) & board.getAttacked(us)) // bonus for moving piece to defended square
    //     v.score += DEFENDED_BONUS;

    return v;
}

/**
 * @brief Sorts moves for qsearch
 *
 * @param board
 * @param moves
 */
inline void SortMovesQ(Board &board, MoveList *moves)
{
    MoveVal vals[moves->size];

    for (int i = 0; i < moves->size; i++)
    {
        vals[i] = ScoreMoveQ(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal &a, const MoveVal &b)
              { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

inline void SortMovesQ(Board &board, MoveList *moves, Move best)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        if (moves->moves[i] == best)
            vals[i] = MoveVal(best, PV_BONUS);
        else
            vals[i] = ScoreMoveQ(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal &a, const MoveVal &b)
              { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

inline void SortMoves(Board &board, MoveList *moves)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        vals[i] = ScoreMove(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal &a, const MoveVal &b)
              { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

inline void SortMoves(Board &board, MoveList *moves, Move prev)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        if (prev == moves->moves[i])
            vals[i] = MoveVal(prev, PV_BONUS);
        else
            vals[i] = ScoreMove(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal &a, const MoveVal &b)
              { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

#endif