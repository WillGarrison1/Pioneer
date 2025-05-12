#include "evaluate.h"

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"

#define DEFENDED_BONUS 10
#define ATTACKED_PENALTY -15
#define REACH_MULTIPLIER 2
#define DOUBLED_PAWNS_PENALTY -20
#define ISOLATED_PAWN_PENALTY -30
#define PASSED_PAWN_BONUS 50
#define MOVING_BONUS 5

/**
 * @brief Evaluates a Piece from White's perspective
 *
 * @param board the board to evaluate
 * @return Score
 */
template <PieceType P>
Score EvalPiece(const Board& board)
{
    Score score = 0;

    Bitboard pieceW = board.getBB(WHITE, P);
    Bitboard pieceB = board.getBB(BLACK, P);

    const Bitboard defendedW = board.getAttacked(WHITE);
    const Bitboard defendedB = board.getAttacked(BLACK);

    Square piece;

    while (pieceW)
    {
        piece = popLSB(pieceW);
        score += GetPSQValue<P, WHITE>(piece);
        if constexpr (P != KING)
        {
            score += defendedW & sqrToBB(piece) ? DEFENDED_BONUS * P : 0;
            score += defendedB & sqrToBB(piece) ? ATTACKED_PENALTY * P : 0;
        }
    }

    while (pieceB)
    {
        piece = popLSB(pieceB);
        score -= GetPSQValue<P, BLACK>(piece);
        if constexpr (P != KING)
        {
            score -= defendedB & sqrToBB(piece) ? DEFENDED_BONUS * P : 0;
            score -= defendedW & sqrToBB(piece) ? ATTACKED_PENALTY * P : 0;
        }
    }

    return score;
}

template <>
Score EvalPiece<PAWN>(const Board& board)
{
    Score score = 0;

    const Bitboard pawnWAll = board.getBB(WHITE, PAWN);
    const Bitboard pawnBAll = board.getBB(BLACK, PAWN);
    Bitboard pawnW = pawnWAll;
    Bitboard pawnB = pawnBAll;

    const Bitboard defendedW = board.getAttacked(WHITE);
    const Bitboard defendedB = board.getAttacked(BLACK);

    // Check for doubled pawns
    for (File file = FILE_A; file <= FILE_H; file++)
    {
        Bitboard fBB = fileBBs[file];
        if (popCount(fBB & pawnW) > 1)
            score += DOUBLED_PAWNS_PENALTY *
                     (popCount(fBB & pawnW) - 1); // multiply penalty by the number of pawns doubled minus one
        if (popCount(fBB & pawnB) > 1)
            score -= DOUBLED_PAWNS_PENALTY *
                     (popCount(fBB & pawnW) - 1); // multiply penalty by the number of pawns doubled minus one
    }

    Square piece;

    while (pawnW)
    {
        piece = popLSB(pawnW);
        score += GetPSQValue<PAWN, WHITE>(piece);
        score += defendedW & sqrToBB(piece) ? DEFENDED_BONUS : 0;
        score += defendedB & sqrToBB(piece) ? ATTACKED_PENALTY : 0;

        bool passedPawn = !(passedPawnBB[WHITE][piece] & pawnBAll);
        bool isolatedPawn = !(isolatedPawnBB[piece] & pawnWAll);

        if (passedPawn)
            score += PASSED_PAWN_BONUS +
                     20 * (getRank(piece) -
                           1); // bonus for passed pawns and extra for the rank it's on (to encorage pushing)
        if (isolatedPawn)
            score += passedPawn ? ISOLATED_PAWN_PENALTY >> 1
                                : ISOLATED_PAWN_PENALTY; // halve isolated penalty if also a passed pawn
    }

    while (pawnB)
    {
        piece = popLSB(pawnB);
        score -= GetPSQValue<PAWN, BLACK>(piece);
        score -= defendedB & sqrToBB(piece) ? DEFENDED_BONUS : 0;
        score -= defendedW & sqrToBB(piece) ? ATTACKED_PENALTY : 0;

        bool passedPawn = !(passedPawnBB[BLACK][piece] & pawnWAll);
        bool isolatedPawn = !(isolatedPawnBB[piece] & pawnBAll);

        if (passedPawn)
            score -=
                PASSED_PAWN_BONUS +
                20 *
                    (6 - getRank(piece)); // bonus for passed pawns and extra for the rank it's on (to encorage pushing)
        if (isolatedPawn)
            score -= passedPawn ? ISOLATED_PAWN_PENALTY >> 1
                                : ISOLATED_PAWN_PENALTY; // halve isolated penalty if also a passed pawn
    }

    return score;
}

Score Eval(Board& board)
{
    Score score = EvalPiece<PAWN>(board) + EvalPiece<KNIGHT>(board) + EvalPiece<BISHOP>(board) +
                  EvalPiece<ROOK>(board) + EvalPiece<QUEEN>(board) + EvalPiece<KING>(board);

    // Add bonus for the amount of squares attacked/defended by each side
    score += (popCount(board.getAttacked(WHITE)) - popCount(board.getAttacked(BLACK))) * REACH_MULTIPLIER;

    score += board.getPawnMaterial() + board.getNonPawnMaterial(); // score material

    score += MOVING_BONUS * (board.whiteToMove ? 1 : -1); // add small bonus to side that is going to move

    return score * (board.whiteToMove ? 1 : -1);
}