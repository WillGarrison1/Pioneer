#include "evaluate.h"

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "profile.h"

#define DEFENDED_BONUS 10    // bonus for defended piece
#define ATTACKED_PENALTY -15 // penalty for attacked piece

#define KING_ATTACKED -10 // penalty for squares attacked adjacent to king

#define PAWN_SHIELD_PENALTY -15   // penalty for each absence of a pawn in from of the king after castling
#define REACH_MULTIPLIER 3        // multiplier for reach (number of defended squares) bonus
#define DOUBLED_PAWNS_PENALTY -20 // penalty for doubled pawns
#define ISOLATED_PAWN_PENALTY -30 // penalty for isolated pawns
#define PASSED_PAWN_BONUS 50      // bonus for passed pawns
#define BISHOP_PAIR_BONUS 40      // bonus for bishop pairs

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
Score EvalPiece<BISHOP>(const Board& board)
{

    Score score = 0;

    Bitboard pieceW = board.getBB(WHITE, BISHOP);
    Bitboard pieceB = board.getBB(BLACK, BISHOP);

    const Bitboard defendedW = board.getAttacked(WHITE);
    const Bitboard defendedB = board.getAttacked(BLACK);

    Square piece;

    score += popCount(pieceW & defendedW) * DEFENDED_BONUS;
    score += popCount(pieceW & defendedB) * ATTACKED_PENALTY;

    score -= popCount(pieceB & defendedB) * DEFENDED_BONUS;
    score -= popCount(pieceB & defendedW) * ATTACKED_PENALTY;

    score += (popCount(pieceW) == 2) * BISHOP_PAIR_BONUS;
    score -= (popCount(pieceB) == 2) * BISHOP_PAIR_BONUS;

    while (pieceW)
    {
        piece = popLSB(pieceW);
        score += GetPSQValue<BISHOP, WHITE>(piece);
    }

    while (pieceB)
    {
        piece = popLSB(pieceB);
        score -= GetPSQValue<BISHOP, BLACK>(piece);
    }

    return score;
}

template <>
Score EvalPiece<KING>(const Board& board)
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

    if ((board.getState()->castling & (CASTLE_WK | CASTLE_WQ)) == 0) // if white can't castle, check for pawn shield
    {
        const Bitboard maskW = pawnShield[0][getFile(pieceW)];

        // get number of possible shielded pawns by getting the width of the mask
        const int totalShieldersW = popCount(maskW & rankBBs[RANK_2]);
        const int numShieldersW = popCount(board.getBB(WHITE, PAWN) & maskW);

        score += std::max(totalShieldersW - numShieldersW, 0) * PAWN_SHIELD_PENALTY;
    }

    if ((board.getState()->castling & (CASTLE_BK | CASTLE_BQ)) == 0) // if black can't castle, check for pawn shield
    {
        const Bitboard maskB = pawnShield[1][getFile(pieceB)];

        // get number of possible shielded pawns by getting the width of the mask
        const int totalShieldersB = popCount(maskB & rankBBs[RANK_7]);
        const int numShieldersB = popCount(board.getBB(BLACK, PAWN) & maskB);

        score -= std::max(totalShieldersB - numShieldersB, 0) * PAWN_SHIELD_PENALTY;
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
            score += PASSED_PAWN_BONUS +
                     20 * (getRank(piece) -
                           RANK_2); // bonus for passed pawns and extra for the rank it's on (to encorage pushing)
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
                20 * (RANK_7 -
                      getRank(piece)); // bonus for passed pawns and extra for the rank it's on (to encorage pushing)
        if (isolatedPawn)
            score -= passedPawn ? ISOLATED_PAWN_PENALTY >> 1
                                : ISOLATED_PAWN_PENALTY; // halve isolated penalty if also a passed pawn
    }

    return score;
}

template <EvalType type>
Score Evaluate(Board& board)
{
    PROFILE_FUNC();

    Score score = EvalPiece<PAWN>(board) + EvalPiece<KNIGHT>(board) + EvalPiece<BISHOP>(board) +
                  EvalPiece<ROOK>(board) + EvalPiece<QUEEN>(board) + EvalPiece<KING>(board);

    // Add bonus for the amount of squares attacked/defended by each side
    score += (popCount(board.getAttacked(WHITE)) - popCount(board.getAttacked(BLACK))) * REACH_MULTIPLIER;

    score += board.getPawnMaterial() + board.getNonPawnMaterial(); // score material

    return score * (board.whiteToMove ? 1 : -1);
}

template <>
Score Eval<FULL>(Board& board)
{
    return Evaluate<FULL>(board);
}

template <>
Score Eval<FAST>(Board& board)
{
    return (board.getPawnMaterial() + board.getNonPawnMaterial()) * (board.whiteToMove ? 1 : -1);
}
