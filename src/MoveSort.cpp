#include "MoveSort.h"

alignas(64) Move killerMoves[MAX_PLY][2]; // each ply can have two killer moves
alignas(64) int moveHistory[2][64][64];   // History for [isBlack][from][to]

MoveVal ScoreMove(const Board& board, Move m)
{
    const Piece piece = board.getSQ(m.from());
    const PieceType pType = getType(piece);
    const Color us = getColor(piece);

    MoveVal v(m, 0);

    if (isKillerMove(board.getPly(), m))
        v.score += KILLER_MOVE_BONUS;

    if (m.type() == CAPTURE) // bonus for captures
    {
        v.score += Mvv_Lva_Score(board, m) + CAPTURE_BONUS;
    }

    if (m.type() == PROMOTION) // bonus for promotions
        v.score += pieceScores[getType(m.promotion())] + PROMOTION_BONUS;

    if (m.isType<QUIET>())
        v.score += moveHistory[board.sideToMove == BLACK][m.from()][m.to()];

    if (sqrToBB(m.to()) & board.getAttacked(~us)) // penalty for moving piece to attacked square
        v.score += (ATTACKED_PENALTY - pieceScores[pType]) >> 1;

    // if (sqrToBB(m.to()) & board.getAttacked(us)) // bonus for moving piece to defended square
    //     v.score += DEFENDED_BONUS + pieceScores[getType(m.piece())] >> 5;

    v.score += us == WHITE ? GetPSQValue<WHITE>(pType, m.to()) - GetPSQValue<WHITE>(pType, m.from())
                           : GetPSQValue<BLACK>(pType, m.to()) - GetPSQValue<BLACK>(pType, m.from());

    return v;
}

MoveVal ScoreMoveQ(const Board& board, Move m)
{
    MoveVal v(m, 0);

    v.score += Mvv_Lva_Score(board, m);

    if (m.type() == PROMOTION) // bonus for promotions
        v.score += pieceScores[getType(m.promotion())] + PROMOTION_BONUS;

    // if (sqrToBB(m.to()) & board.getAttacked(~us)) // penalty for moving piece to attacked square
    //     v.score += ATTACKED_PENALTY;

    // if (sqrToBB(m.to()) & board.getAttacked(us)) // bonus for moving piece to defended square
    //     v.score += DEFENDED_BONUS;

    return v;
}

void SortMovesQ(const Board& board, MoveList* moves)
{
    MoveVal vals[moves->size];

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

void SortMovesQ(const Board& board, MoveList* moves, Move best)
{
    MoveVal vals[moves->size];

    for (int i = 0; i < moves->size; i++)
    {
        if (moves->moves[i] == best)
            vals[i] = MoveVal(best, PV_BONUS);
        else
            vals[i] = ScoreMoveQ(board, moves->moves[i]);
    }

    std::sort(vals, vals + moves->size, [](const MoveVal& a, const MoveVal& b) { return a.score > b.score; });

    for (int i = 0; i < moves->size; i++)
    {
        moves->moves[i] = vals[i].m;
    }
}

void SortMoves(const Board& board, MoveList* moves)
{
    MoveVal vals[moves->size];

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

void SortMoves(const Board& board, MoveList* moves, Move prev)
{
    MoveVal vals[moves->size];

    for (int i = 0; i < moves->size; i++)
    {
        if (prev == moves->moves[i])
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