#include "evaluate.h"

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "profile.h"

#define DEFENDED_BONUS 10    // bonus for defended piece
#define ATTACKED_PENALTY -15 // penalty for attacked piece

#define KING_ATTACKED -15 // penalty for squares attacked adjacent to king

#define REACH_MULTIPLIER 1        // multiplier for reach (number of defended squares) bonue
#define DOUBLED_PAWNS_PENALTY -20 // penalty for doubled pawns
#define ISOLATED_PAWN_PENALTY -30 // penalty for isolated pawns
#define PASSED_PAWN_BONUS 50      // bonus for passed pawns
#define MOVING_BONUS 5            // small bonus for moving side

/**
 * @brief Evaluates a Piece from White's perspective
 *
 * @param board the board to evaluate
 * @return Score
 */
template <PieceType P>
Score EvalPiece(const Board &board)
{

    Score score = 0;

    Bitboard pieceW = board.getBB(WHITE, P);
    Bitboard pieceB = board.getBB(BLACK, P);

    const Bitboard defendedW = board.getAttacked(WHITE);
    const Bitboard defendedB = board.getAttacked(BLACK);

    Square piece;

    score += popCount(pieceW & defendedW) * DEFENDED_BONUS;
    score += popCount(pieceW & defendedB) * ATTACKED_PENALTY;

    score -= popCount(pieceB & defendedB) * DEFENDED_BONUS;
    score -= popCount(pieceB & defendedW) * ATTACKED_PENALTY;

    while (pieceW)
    {
        piece = popLSB(pieceW);
        score += GetPSQValue<P, WHITE>(piece);
    }

    while (pieceB)
    {
        piece = popLSB(pieceB);
        score -= GetPSQValue<P, BLACK>(piece);
    }

    return score;
}

template <>
Score EvalPiece<KING>(const Board &board)
{
    Score score = 0;

    const Square pieceW = lsb(board.getBB(WHITE, KING));
    const Square pieceB = lsb(board.getBB(BLACK, KING));

    const Bitboard defendedW = board.getAttacked(WHITE);
    const Bitboard defendedB = board.getAttacked(BLACK);

    // Evaluate king safety by number of squares attacked adjacent to king
    score += GetPSQValue<KING, WHITE>(pieceW);
    score -= GetPSQValue<KING, BLACK>(pieceB);

    Bitboard attackedKingWSquares = kingMoves[pieceW] & defendedB;

    score += popCount(attackedKingWSquares) * KING_ATTACKED;

    Bitboard attackedKingBSquares = kingMoves[pieceB] & defendedW;

    score -= popCount(attackedKingBSquares) * KING_ATTACKED;

    return score;
}

template <>
Score EvalPiece<PAWN>(const Board &board)
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
                     (popCount(fBB & pawnB) - 1); // multiply penalty by the number of pawns doubled minus one
    }

    Square piece;

    score += popCount(pawnWAll & defendedW) * DEFENDED_BONUS;
    score += popCount(pawnWAll & defendedB) * ATTACKED_PENALTY;

    score -= popCount(pawnBAll & defendedB) * DEFENDED_BONUS;
    score -= popCount(pawnBAll & defendedW) * ATTACKED_PENALTY;

    while (pawnW)
    {
        piece = popLSB(pawnW);
        score += GetPSQValue<PAWN, WHITE>(piece);

        bool passedPawn = !(passedPawnBB[WHITE][piece] & pawnBAll);
        bool isolatedPawn = !(isolatedPawnBB[piece] & pawnWAll);

        if (passedPawn)
            score +=
                PASSED_PAWN_BONUS +
                20 *
                    (6 - getRank(piece)); // bonus for passed pawns and extra for the rank it's on (to encorage pushing)
        if (isolatedPawn)
            score += passedPawn ? ISOLATED_PAWN_PENALTY >> 1
                                : ISOLATED_PAWN_PENALTY; // halve isolated penalty if also a passed pawn
    }

    while (pawnB)
    {
        piece = popLSB(pawnB);
        score -= GetPSQValue<PAWN, BLACK>(piece);

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

template <EvalType type>
Score Evaluate(Board &board)
{
    PROFILE_FUNC();

    Score score = EvalPiece<PAWN>(board) + EvalPiece<KNIGHT>(board) + EvalPiece<BISHOP>(board) +
                  EvalPiece<ROOK>(board) + EvalPiece<QUEEN>(board) + EvalPiece<KING>(board);

    // Add bonus for the amount of squares attacked/defended by each side
    score += (popCount(board.getAttacked(WHITE)) - popCount(board.getAttacked(BLACK))) * REACH_MULTIPLIER;

    score += board.getPawnMaterial() + board.getNonPawnMaterial(); // score material

    score += MOVING_BONUS * (board.whiteToMove ? 1 : -1); // add small bonus to side that is going to move

    return score * (board.whiteToMove ? 1 : -1);
}

template <>
Score Eval<FULL>(Board &board)
{
    return Evaluate<FULL>(board);
}

template <>
Score Eval<FAST>(Board &board)
{
    return Evaluate<FAST>(board);
}
