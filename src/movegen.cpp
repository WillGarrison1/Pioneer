#include <iostream>
#include <stdexcept>

#include "movegen.h"

#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "direction.h"
#include "magic.h"
#include "square.h"
#include "profile.h"

struct MovegenMasks
{
    Bitboard checkBB; // check bitboard
    Bitboard pinnedS; // straight pins
    Bitboard pinnedD; // diagonal pins
};

// Special pin detection for en passant
Direction isPinned(const Board &board, Square s, Square enPassant)
{
    const Bitboard kingBB = board.getBB(board.sideToMove, KING);
    const Bitboard blockers = board.getBB(ALL_PIECES) & ~sqrToBB(enPassant);
    const Direction dir = directionsTable[s][lsb(kingBB)];
    Bitboard open = 0ULL;

    // Piece is not in any sliding direction from king
    if (dir == NONE_DIR)
        return NONE_DIR;

    open = bitboardPaths[s][lsb(kingBB)] & blockers & ~kingBB;
    if (!open) // Does piece have line of sight to king? (bitboard is empty)
    {
        open = (GetBishopMoves(blockers, s) | GetRookMoves(blockers, s)) & bitboardRays[-dir][s];

        switch (dir)
        {
        case NORTH:
        case SOUTH:
        case EAST:
        case WEST:
            if (open & board.getBB(~board.sideToMove, ROOK, QUEEN) & ~sqrToBB(enPassant))
                return -dir; // return if piece also sees a rook/queen
            return NONE_DIR;
        case NORTH_EAST:
        case NORTH_WEST:
        case SOUTH_EAST:
        case SOUTH_WEST:
            if (open & board.getBB(~board.sideToMove, BISHOP, QUEEN) & ~sqrToBB(enPassant))
                return -dir; // return if piece also sees a bishop/queen
            return NONE_DIR;
        default:
            break;
        }
    }
    return NONE_DIR;
}

// Pawn move generation
template <MoveType mType>
void generatePawnMoves(const Board &board, MoveList *list, MovegenMasks *masks)
{
    PROFILE_FUNC();

    const Direction forward = board.whiteToMove ? NORTH : SOUTH;
    const Direction doubleForward = forward << 1;

    const Bitboard pawns = board.getBB(board.sideToMove, PAWN);
    const Bitboard empty = board.getBB(EMPTY);
    const Bitboard enemy = board.getBB(~board.sideToMove);

    Bitboard pinnedPawnsS = pawns & masks->pinnedS; // these pawns can't attack but might be able to move forward
    Bitboard pinnedPawnsD = pawns & masks->pinnedD; // these pawns can't move forward but can attack

    Bitboard unpinnedPawnsS = pawns & ~masks->pinnedS;
    Bitboard unpinnedPawnsD = pawns & ~masks->pinnedD;

    if constexpr (mType == ALL_MOVES)
    {
        Bitboard singlePushesUnpinned = shift(unpinnedPawnsS & ~masks->pinnedD, forward);
        Bitboard singlePushesPinned = shift(pinnedPawnsS & ~masks->pinnedD, forward) & masks->pinnedS;
        Bitboard singlePushes = (singlePushesPinned | singlePushesUnpinned) & empty; // pawns can't move forward if pinned diagonally

        Bitboard doublePushes = shift(singlePushes, forward) & empty & (board.whiteToMove ? rankBBs[RANK_4] : rankBBs[RANK_5]);
        singlePushes &= masks->checkBB;
        doublePushes &= masks->checkBB;

        while (singlePushes)
        {
            Square to = popLSB(singlePushes);
            Square from = to - forward;

            if (to >= SQ_A8 || to <= SQ_H1)
            {
                list->addMove(board.createMove(from, to, PROMOTION, QUEEN));
                list->addMove(board.createMove(from, to, PROMOTION, ROOK));
                list->addMove(board.createMove(from, to, PROMOTION, BISHOP));
                list->addMove(board.createMove(from, to, PROMOTION, KNIGHT));
                continue;
            }

            list->addMove(board.createMove(from, to));
        }

        while (doublePushes)
        {
            Square to = popLSB(doublePushes);
            Square from = to - doubleForward;

            list->addMove(board.createMove(from, to));
        }
    }

    Bitboard attacksWestPinned = shift(pinnedPawnsD & ~fileBBs[FILE_A] & ~masks->pinnedS, forward + WEST) & masks->pinnedD;
    Bitboard attacksWestUnpinned = shift(unpinnedPawnsD & ~fileBBs[FILE_A] & ~masks->pinnedS, forward + WEST);

    Bitboard attacksWest = (attacksWestPinned | attacksWestUnpinned) & enemy & masks->checkBB;

    while (attacksWest)
    {
        Square to = popLSB(attacksWest);
        Square from = to - (forward + WEST);

        if (to >= SQ_A8 || to <= SQ_H1)
        {
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, QUEEN));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, ROOK));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, BISHOP));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, KNIGHT));
            continue;
        }

        list->addMove(board.createMove(from, to, CAPTURE));
    }

    Bitboard attacksEastPinned = shift(pinnedPawnsD & ~fileBBs[FILE_H] & ~masks->pinnedS, forward + EAST) & masks->pinnedD;
    Bitboard attacksEastUnpinned = shift(unpinnedPawnsD & ~fileBBs[FILE_H] & ~masks->pinnedS, forward + EAST);

    Bitboard attacksEast = (attacksEastPinned | attacksEastUnpinned) & enemy & masks->checkBB;

    while (attacksEast)
    {
        Square to = popLSB(attacksEast);
        Square from = to - (forward + EAST);

        if (to >= SQ_A8 || to <= SQ_H1)
        {
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, QUEEN));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, ROOK));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, BISHOP));
            list->addMove(board.createMove(from, to, PROMOTION | CAPTURE, KNIGHT));
            continue;
        }

        list->addMove(board.createMove(from, to, CAPTURE));
    }

    // En Passant
    if (board.getEnPassantSqr() != SQ_NONE)
    {
        Bitboard attacksWest = shift(pawns & ~fileBBs[FILE_A], forward + WEST) &
                               shift(masks->checkBB, forward) & sqrToBB(board.getEnPassantSqr());
        Bitboard attacksEast = shift(pawns & ~fileBBs[FILE_H], forward + EAST) &
                               shift(masks->checkBB, forward) & sqrToBB(board.getEnPassantSqr());
        Square enPassantAttacked = board.getEnPassantSqr() - forward;
        if (attacksEast)
        {
            Square to = lsb(attacksEast);
            Square from = to - (forward + EAST);
            Direction pinned = isPinned(board, from, enPassantAttacked);
            if (!(pinned != NONE_DIR && pinned != forward + EAST && pinned != -forward + WEST))
                // piece is pinned
                list->addMove(board.createMove(from, to, EN_PASSANT | CAPTURE));
        }
        if (attacksWest)
        {
            Square to = lsb(attacksWest);
            Square from = to - (forward + WEST);
            Direction pinned = isPinned(board, from, enPassantAttacked);
            if (!(pinned != NONE_DIR && pinned != forward + WEST && pinned != -forward + EAST))
                // piece is pinned
                list->addMove(board.createMove(from, to, EN_PASSANT | CAPTURE));
        }
    }
}

// Knight move generation

template <MoveType mType>
void generateKnightMoves(const Board &board, MoveList *list, MovegenMasks *masks)
{
    PROFILE_FUNC();

    Bitboard knights = board.getBB(board.sideToMove, KNIGHT) & ~(masks->pinnedS | masks->pinnedD); // doesn't matter which way the pin is
    while (knights)
    {
        Square from = popLSB(knights);

        Bitboard moves = knightMoves[from] & masks->checkBB;

        Bitboard captures = moves & board.getBB(~board.sideToMove);
        Bitboard quiets = moves & ~board.getBB(ALL_PIECES);

        while (quiets)
        {
            Square to = popLSB(quiets);
            list->addMove(board.createMove(from, to, QUIET));
        }

        while (captures)
        {
            Square to = popLSB(captures);
            list->addMove(board.createMove(from, to, CAPTURE));
        }
    }
}

template <MoveType mType>
void generateBishopMoves(const Board &board, MoveList *list, MovegenMasks *masks)
{
    PROFILE_FUNC();

    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard bishops = board.getBB(board.sideToMove, BISHOP) & ~masks->pinnedS;

    while (bishops)
    {
        Square from = popLSB(bishops);
        Bitboard pinnedBB = -1ULL;

        if (masks->pinnedD & sqrToBB(from))
            pinnedBB = masks->pinnedD;

        Bitboard moves = GetBishopMoves(blockers, from) & open & pinnedBB & masks->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateRookMoves(const Board &board, MoveList *list, MovegenMasks *masks)
{
    PROFILE_FUNC();
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard rooks = board.getBB(board.sideToMove, ROOK) & ~masks->pinnedD;

    while (rooks)
    {
        Square from = popLSB(rooks);
        Bitboard pinnedBB = -1ULL;

        if (masks->pinnedS & sqrToBB(from))
            pinnedBB = masks->pinnedS;

        Bitboard moves = GetRookMoves(blockers, from) & open & pinnedBB & masks->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateQueenMoves(const Board &board, MoveList *list, MovegenMasks *masks)
{
    PROFILE_FUNC();

    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard queens = board.getBB(board.sideToMove, QUEEN);

    while (queens)
    {
        Square from = popLSB(queens);
        Bitboard moves;

        if (sqrToBB(from) & masks->pinnedD)
        {
            moves = masks->pinnedD & GetBishopMoves(blockers, from);
        }
        else if (sqrToBB(from) & masks->pinnedS)
        {
            moves = masks->pinnedS & GetRookMoves(blockers, from);
        }
        else
        {
            moves = GetRookMoves(blockers, from) | GetBishopMoves(blockers, from);
        }

        moves &= open & masks->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateKingMoves(const Board &board, MoveList *list)
{
    PROFILE_FUNC();
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove)) &
                          ~board.getAttacked(~board.sideToMove);
    const Square from = lsb(board.getBB(board.sideToMove, KING));

    Bitboard moves = kingMoves[from] & open;
    while (moves)
    {
        Square to = popLSB(moves);
        list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
    }
}

void generateCastlingMoves(const Board &board, MoveList *list)
{
    PROFILE_FUNC();
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const CastlingRights castleRights = board.getState()->castling;

    const Bitboard open = ~(blockers | board.getAttacked(~board.sideToMove));

    if (board.sideToMove == WHITE)
    {
        // Short castle
        if ((castleRights & CASTLE_WK) &&                          // has castle right
            (open & castleBBs[CASTLE_WK]) == castleBBs[CASTLE_WK]) // no obstructions between king and rook
        {
            list->addMove(board.createMove(SQ_E1, SQ_G1, CASTLE, EMPTY, CASTLE_WK));
        }
        // Long castle
        if ((castleRights & CASTLE_WQ) &&                              // has castle right
            ((open & castleBBs[CASTLE_WQ]) == castleBBs[CASTLE_WQ]) && // no attacked squares or pieces where king moves through
            (blockers & sqrToBB(SQ_B1)) == 0ULL)                       // no piecce next to rook
        {
            list->addMove(board.createMove(SQ_E1, SQ_C1, CASTLE, EMPTY, CASTLE_WQ));
        }
    }
    else
    {
        // Short castle
        if ((castleRights & CASTLE_BK) &&
            ((open & castleBBs[CASTLE_BK]) == castleBBs[CASTLE_BK]))
        {
            list->addMove(board.createMove(SQ_E8, SQ_G8, CASTLE, EMPTY, CASTLE_BK));
        }
        // Long castle
        if ((castleRights & CASTLE_BQ) &&
            ((open & castleBBs[CASTLE_BQ]) == castleBBs[CASTLE_BQ]) &&
            (blockers & sqrToBB(SQ_B8)) == 0ULL)
        {
            list->addMove(board.createMove(SQ_E8, SQ_C8, CASTLE, EMPTY, CASTLE_BQ));
        }
    }
}

template <MoveType type>
void generateMoves(Board &board, MoveList *list)
{
    PROFILE_FUNC();
    assert(board.getBB(board.sideToMove, KING));

    Bitboard checkBB = board.computeAttackedBBs();
    Bitboard pinnedS;
    Bitboard pinnedD;
    board.computePins(pinnedS, pinnedD);
    MovegenMasks masks = {checkBB, pinnedS, pinnedD};

    if (board.getNumChecks() < 2)
    {
        generatePawnMoves<type>(board, list, &masks);
        generateKnightMoves<type>(board, list, &masks);
        generateBishopMoves<type>(board, list, &masks);
        generateRookMoves<type>(board, list, &masks);
        generateQueenMoves<type>(board, list, &masks);

        if constexpr (type == ALL_MOVES)
        {
            if (board.getNumChecks() == 0)
                generateCastlingMoves(board, list);
        }
    }
    generateKingMoves<type>(board, list);
}

template void generateMoves<ALL_MOVES>(Board &board, MoveList *list);
template void generateMoves<CAPTURE>(Board &board, MoveList *list);