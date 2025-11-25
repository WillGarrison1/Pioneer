#include "MoveSort.h"

alignas(64) Move killerMoves[MAX_PLY][2]; // each ply can have two killer moves
alignas(64) int moveHistory[2][64][64];   // History for [isBlack][from][to]
alignas(64) Move counterMove[64][64];

MoveVal ScoreMove(const Board &board, Move m)
{
    const Piece piece = board.getSQ(m.from());
    const PieceType pType = getType(piece);
    const Color us = getColor(piece);
    const Move prevMove = board.getState()->move;

    MoveVal v = {m, 0};

    if (counterMove[prevMove.from()][prevMove.to()] == m)
    {
        v.score += COUNTERMOVE_BONUS;
    }

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
        v.score += ATTACKED_PENALTY - (pieceScores[pType] >> 7);

    v.score += us == WHITE ? GetPSQValue<WHITE>(pType, m.to()) - GetPSQValue<WHITE>(pType, m.from())
                           : GetPSQValue<BLACK>(pType, m.to()) - GetPSQValue<BLACK>(pType, m.from());

    return v;
}

MoveVal ScoreMoveQ(const Board &board, Move m)
{
    MoveVal v = {m, 0};
    Piece piece = board.getSQ(m.from());
    PieceType pType = getType(piece);

    v.score += Mvv_Lva_Score(board, m);

    if (m.type() == PROMOTION) // bonus for promotions
        v.score += pieceScores[getType(m.promotion())] + PROMOTION_BONUS;

    if (sqrToBB(m.to()) & board.getAttacked(~board.sideToMove)) // penalty for moving piece to attacked square
        v.score += ATTACKED_PENALTY - (pieceScores[pType] >> 3);

    return v;
}