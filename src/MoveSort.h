#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#include <algorithm>

#define CAPTURE_BONUS 200
#define PROMOTION_BONUS 500
#define DEFENDED_BONUS 25
#define ATTACKED_PENALTY -35
#define PV_BONUS 1000
#define CHECK_BONUS 20

struct MoveVal
{
    Move m;
    int score;

    MoveVal() = default;

    MoveVal(Move m, int score) : m(m), score(score)
    {
    }
};

inline MoveVal ScoreMove(Board& board, Move m)
{
    MoveVal v(m, 0);

    if (m.type() & CAPTURE) // bonus for captures
        v.score += static_cast<int>(getType(m.captured())) - static_cast<int>(getType(m.piece())) + CAPTURE_BONUS;

    if (m.type() & PROMOTION) // bonus for promotions
        v.score += static_cast<int>(getType(m.promotion())) + PROMOTION_BONUS;

    if (sqrToBB(m.to()) & board.getAttacked(~board.sideToMove)) // penalty for moving piece to attacked square
        v.score += ATTACKED_PENALTY - 5 * getType(m.piece());

    if (sqrToBB(m.to()) & board.getAttacked(board.sideToMove)) // bonus for moving piece to defended square
        v.score += DEFENDED_BONUS + 3 * getType(m.piece());

    v.score += board.whiteToMove
                   ? GetPSQValue<WHITE>(getType(m.piece()), m.to()) - GetPSQValue<WHITE>(getType(m.piece()), m.from())
                   : GetPSQValue<BLACK>(getType(m.piece()), m.to()) - GetPSQValue<BLACK>(getType(m.piece()), m.from());

    return v;
}

inline MoveVal ScoreMoveQ(Board& board, Move m)
{
    MoveVal v(m, 0);

    v.score += static_cast<int>(getType(m.captured())) - static_cast<int>(getType(m.piece())) + CAPTURE_BONUS;

    if (m.type() & PROMOTION) // bonus for promotions
        v.score += static_cast<int>(getType(m.promotion())) + PROMOTION_BONUS;

    if (sqrToBB(m.to()) & board.getAttacked(~board.sideToMove)) // penalty for moving piece to attacked square
        v.score += ATTACKED_PENALTY;

    if (sqrToBB(m.to()) & board.getAttacked(board.sideToMove)) // bonus for moving piece to defended square
        v.score += DEFENDED_BONUS;

    return v;
}

/**
 * @brief Sorts moves for qsearch
 *
 * @param board
 * @param moves
 */
inline void SortMovesQ(Board& board, MoveList* moves)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        vals[i] = ScoreMoveQ(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal& a, const MoveVal& b) { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

inline void SortMoves(Board& board, MoveList* moves)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        vals[i] = ScoreMove(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal& a, const MoveVal& b) { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

inline void SortMoves(Board& board, MoveList* moves, Move prev)
{
    MoveVal vals[256];

    for (int i = 0; i < moves->size; i++)
    {
        if (prev.getMove() == moves->moves[i].getMove())
            vals[i] = MoveVal(prev, PV_BONUS);
        else
            vals[i] = ScoreMove(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal& a, const MoveVal& b) { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

#endif