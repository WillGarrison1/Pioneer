#include <iostream>
#include <stdexcept>

#include "movegen.h"

#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "direction.h"
#include "magic.h"
#include "square.h"

Direction isPinned(const Board& board, Square s)
{
    const Bitboard kingBB = board.getBB(board.sideToMove, KING);
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Direction dir = directionsTable[s][lsb(kingBB)];
    Bitboard open = 0ULL;

    // Piece is not in any sliding direction from king
    if (dir == NONE_DIR)
        return NONE_DIR;

    open = bitboardPaths[s][lsb(kingBB)] & blockers & ~kingBB;
    if (!open) // Does piece have line of sight to king?
    {
        open = (GetBishopMoves(blockers, s) | GetRookMoves(blockers, s)) & bitboardRays[-dir][s];
        switch (dir)
        {
        case NORTH:
        case SOUTH:
        case EAST:
        case WEST:
            if (open & board.getBB(~board.sideToMove, ROOK, QUEEN))
                return -dir; // return if piece also sees a rook/queen
            return NONE_DIR;
        case NORTH_EAST:
        case NORTH_WEST:
        case SOUTH_EAST:
        case SOUTH_WEST:
            if (open & board.getBB(~board.sideToMove, BISHOP, QUEEN))
                return -dir; // return if piece also sees a bishop/queen
            return NONE_DIR;
        default:
            break;
        }
    }
    return NONE_DIR;
}

Direction isPinned(const Board& board, Square s, Square enPassant)
{
    const Bitboard kingBB = board.getBB(board.sideToMove, KING);
    const Bitboard blockers = board.getBB(ALL_PIECES) & ~sqrToBB(enPassant);
    const Direction dir = directionsTable[s][lsb(kingBB)];
    Bitboard open = 0ULL;

    // Piece is not in any sliding direction from king
    if (dir == NONE_DIR)
        return NONE_DIR;

    open = bitboardPaths[s][lsb(kingBB)] & blockers & ~kingBB;
    if (!open) // Does piece have line of sight to king?
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
void generatePawnMoves(const Board& board, MoveList* list)
{
    const Direction forward = board.whiteToMove ? NORTH : SOUTH;
    const Direction doubleForward = forward << 1;
    const Bitboard pawns = board.getBB(board.sideToMove, PAWN);
    const Bitboard empty = board.getBB(EMPTY);
    const Bitboard enemy = board.getBB(~board.sideToMove);

    Bitboard attacksWest = shift(pawns & ~fileBBs[FILE_A], forward + WEST) & enemy & board.getState()->checkBB;
    Bitboard attacksEast = shift(pawns & ~fileBBs[FILE_H], forward + EAST) & enemy & board.getState()->checkBB;

    if constexpr (mType == ALL_MOVES)
    {
        Bitboard singlePushes = shift(pawns, forward) & empty;
        Bitboard doublePushes =
            shift(singlePushes, forward) & empty & (board.whiteToMove ? rankBBs[RANK_4] : rankBBs[RANK_5]);
        singlePushes &= board.getState()->checkBB;
        doublePushes &= board.getState()->checkBB;

        while (singlePushes)
        {
            Square to = popLSB(singlePushes);
            Square from = to - forward;
            Direction pinned = isPinned(board, from);
            if (pinned != NONE_DIR && pinned != NORTH && pinned != SOUTH)
            {
                // piece is pinned, so remove double pushes
                clearBit(doublePushes, to + forward);
                continue;
            }

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

            // Check if piece is pinned if king is in check
            // This is because we might not have checked for pins in the single push
            if (board.getState()->checkBB != fullBB)
            {
                Direction pinned = isPinned(board, from);
                if (pinned != NONE_DIR && pinned != NORTH && pinned != SOUTH)
                    continue;
            }

            list->addMove(board.createMove(from, to));
        }
    }

    while (attacksWest)
    {
        Square to = popLSB(attacksWest);
        Square from = to - (forward + WEST);
        Direction pinned = isPinned(board, from);
        if (pinned != NONE_DIR && pinned != forward + WEST && pinned != -forward + EAST)
            // piece is pinned
            continue;

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

    while (attacksEast)
    {
        Square to = popLSB(attacksEast);
        Square from = to - (forward + EAST);
        Direction pinned = isPinned(board, from);
        if (pinned != NONE_DIR && pinned != forward + EAST && pinned != -forward + WEST)
            // piece is pinned
            continue;

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
                               shift(board.getState()->checkBB, forward) & sqrToBB(board.getEnPassantSqr());
        Bitboard attacksEast = shift(pawns & ~fileBBs[FILE_H], forward + EAST) &
                               shift(board.getState()->checkBB, forward) & sqrToBB(board.getEnPassantSqr());
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
void generateKnightMoves(const Board& board, MoveList* list)
{
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));

    Bitboard knights = board.getBB(board.sideToMove, KNIGHT);
    while (knights)
    {
        Square from = popLSB(knights);
        if (isPinned(board, from) != NONE_DIR)
            continue;
        Bitboard moves = knightMoves[from] & open & board.getState()->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateBishopMoves(const Board& board, MoveList* list)
{
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard bishops = board.getBB(board.sideToMove, BISHOP);

    while (bishops)
    {
        Square from = popLSB(bishops);
        Bitboard pinnedBB = -1ULL;
        Direction pinned = isPinned(board, from);

        if (pinned)
            pinnedBB = bitboardRays[pinned][from] | bitboardRays[-pinned][from];

        Bitboard moves = GetBishopMoves(blockers, from) & open & pinnedBB & board.getState()->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateRookMoves(const Board& board, MoveList* list)
{
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard rooks = board.getBB(board.sideToMove, ROOK);

    while (rooks)
    {
        Square from = popLSB(rooks);
        Bitboard pinnedBB = -1ULL;
        Direction pinned = isPinned(board, from);

        if (pinned)
            pinnedBB = bitboardRays[pinned][from] | bitboardRays[-pinned][from];

        Bitboard moves = GetRookMoves(blockers, from) & open & pinnedBB & board.getState()->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateQueenMoves(const Board& board, MoveList* list)
{
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard open = (mType == ALL_MOVES ? ~board.getBB(board.sideToMove) : board.getBB(~board.sideToMove));
    Bitboard queens = board.getBB(board.sideToMove, QUEEN);

    while (queens)
    {
        Square from = popLSB(queens);
        Bitboard pinnedBB = -1ULL;
        Direction pinned = isPinned(board, from);

        if (pinned)
            pinnedBB = bitboardRays[pinned][from] | bitboardRays[-pinned][from];
        Bitboard moves = (GetRookMoves(blockers, from) | GetBishopMoves(blockers, from)) & open & pinnedBB &
                         board.getState()->checkBB;
        while (moves)
        {
            Square to = popLSB(moves);
            list->addMove(board.createMove(from, to, (MoveType)((board.getBB(~board.sideToMove) & sqrToBB(to)) != 0)));
        }
    }
}

template <MoveType mType>
void generateKingMoves(const Board& board, MoveList* list)
{
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

void generateCastlingMoves(const Board& board, MoveList* list)
{
    const Square king = lsb(board.getBB(board.sideToMove, KING));
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const CastlingRights castleRights = board.getState()->castling;

    if (board.sideToMove == WHITE)
    {
        // Short castle
        if ((castleRights & CASTLE_WK) &&
            ((~(blockers | board.getAttacked(~board.sideToMove))) & castleBBs[CASTLE_WK]) == castleBBs[CASTLE_WK])
        {
            list->addMove(board.createMove(SQ_E1, SQ_G1, CASTLE, EMPTY, CASTLE_WK));
        }
        // Long castle
        if ((castleRights & CASTLE_WQ) &&
            ((~(blockers | board.getAttacked(~board.sideToMove)) & castleBBs[CASTLE_WQ]) == castleBBs[CASTLE_WQ]) &&
            (blockers & sqrToBB(SQ_B1)) == 0ULL)
        {
            list->addMove(board.createMove(SQ_E1, SQ_C1, CASTLE, EMPTY, CASTLE_WQ));
        }
    }
    else
    {
        // Short castle
        if ((castleRights & CASTLE_BK) &&
            ((~(blockers | board.getAttacked(~board.sideToMove)) & castleBBs[CASTLE_BK]) == castleBBs[CASTLE_BK]))
        {
            list->addMove(board.createMove(SQ_E8, SQ_G8, CASTLE, EMPTY, CASTLE_BK));
        }
        // Long castle
        if ((castleRights & CASTLE_BQ) &&
            ((~(blockers | board.getAttacked(~board.sideToMove)) & castleBBs[CASTLE_BQ]) == castleBBs[CASTLE_BQ]) &&
            (blockers & sqrToBB(SQ_B8)) == 0ULL)
        {
            list->addMove(board.createMove(SQ_E8, SQ_C8, CASTLE, EMPTY, CASTLE_BQ));
        }
    }
}

template <MoveType type>
void generateMoves(Board& board, MoveList* list)
{
    assert(board.getBB(board.sideToMove, KING));

    if (board.getNumChecks() < 2)
    {
        generatePawnMoves<type>(board, list);
        generateKnightMoves<type>(board, list);
        generateBishopMoves<type>(board, list);
        generateRookMoves<type>(board, list);
        generateQueenMoves<type>(board, list);

        if constexpr (type == ALL_MOVES)
        {
            if (board.getNumChecks() == 0)
                generateCastlingMoves(board, list);
        }
    }
    generateKingMoves<type>(board, list);
}

template void generateMoves<ALL_MOVES>(Board& board, MoveList* list);
template void generateMoves<CAPTURE>(Board& board, MoveList* list);