#include "MoveSort.h"

alignas(64) Move killerMoves[MAX_PLY][2]; // each ply can have two killer moves
alignas(64) Move counterMove[64][64];
alignas(64) int moveHistory[2][64][64];                  // History for [isBlack][from][to]
alignas(64) int captureHistory[64][64][PieceType::KING]; // indexed as [from][to][victimPieceType-1]
alignas(64) int continuationHistory[CONTINUATION_HISTORY_SIZE][6][64][6][64];

MoveVal ScoreMove(const Board& board, Move m)
{
    const Piece piece = board.getSQ(m.from());
    const PieceType pType = getType(piece);
    const Color us = getColor(piece);
    const Move prevMove =
        board.getState()->move; // this is the previous move because we haven't made the current move yet

    MoveVal v = {m, 0};

    if (m.type() == PROMOTION)
    {
        v.score = pieceScores[getType(m.promotion())] + PROMOTION_BONUS;
    }
    else if (killerMoves[board.getPly()][0] == m)
    {
        v.score = KILLER_MOVE_BONUS;
    }
    else if (killerMoves[board.getPly()][1] == m)
    {
        v.score = KILLER_MOVE_BONUS - 1000;
    }
    else if (counterMove[prevMove.from()][prevMove.to()] == m)
    {
        v.score = COUNTERMOVE_BONUS;
    }

    else if (m.isType<QUIET>())
    {
        v.score += moveHistory[board.sideToMove == BLACK][m.from()][m.to()] * 2;
        const BoardState* prevState = board.getState();
        PieceType moved = getType(board.getSQ(m.from()));
        for (int i = 0; i < CONTINUATION_HISTORY_SIZE; i++)
        {
            if (!prevState || prevState->moved == EMPTY)
                break;
            v.score += continuationHistory[i][getType(prevState->moved) - 1][prevState->move.to()][moved - 1][m.to()];

            prevState = prevState->prev;
        }
        if (sqrToBB(m.to()) & board.getAttacked(~us)) // penalty for moving piece to attacked square
            v.score += ATTACKED_PENALTY - pieceScores[pType];

        v.score += us == WHITE ? GetPSQValue<WHITE>(pType, m.to()) - GetPSQValue<WHITE>(pType, m.from())
                               : GetPSQValue<BLACK>(pType, m.to()) - GetPSQValue<BLACK>(pType, m.from());
    }

    return v;
}

MoveVal ScoreMoveQ(const Board& board, Move m)
{
    MoveVal v = {m, 0};
    PieceType pType = getType(board.getSQ(m.from()));

    PieceType victimType = getType(board.getSQ(m.to()));

    if (m.to() == board.getEnPassantSqr())
        victimType = PAWN;

    v.score += 2 * captureHistory[m.from()][m.to()][victimType - 1] + pieceScores[victimType] * 4;

    v.score += Mvv_Lva_Score(board, m) + CAPTURE_BONUS; 

    if (sqrToBB(m.to()) & board.getAttacked(~board.sideToMove)) // penalty for moving piece to attacked square
        v.score += ATTACKED_PENALTY - pieceScores[pType];

    const BoardState* prevState = board.getState();
    PieceType moved = getType(board.getSQ(m.from()));
    for (int i = 0; i < CONTINUATION_HISTORY_SIZE; i++)
    {
        if (!prevState || prevState->moved == EMPTY)
            break;
        v.score += continuationHistory[i][getType(prevState->moved) - 1][prevState->move.to()][moved - 1][m.to()];

        prevState = prevState->prev;
    }

    return v;
}