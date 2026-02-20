#include <iostream>
#include <stdexcept>

#include "movegen.h"

#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "direction.h"
#include "magic.h"
#include "profile.h"
#include "square.h"

struct MovegenMasks
{
    Bitboard checkBB; // check bitboard
    Bitboard pinnedS; // straight pins
    Bitboard pinnedD; // diagonal pins
};

template <MoveType mType>
inline void BitboardToMoves(const Square from, Bitboard bb, MoveList* list)
{
    while (bb)
    {
        const Square to = popLSB(bb);
        list->addMove<mType>(from, to);
    }
}

// Special pin detection for en passant
template <Color color>
Direction isPinned(const Board& board, Square s, Square enPassant)
{
    const Bitboard kingBB = board.getBB(color, KING);
    const Bitboard blockers = board.getBB(ALL_PIECES) & ~sqrToBB(enPassant);
    const Direction dir = directionsTable[s][lsb(kingBB)];

    // Piece is not in any sliding direction from king
    if (dir == NONE_DIR)
        return NONE_DIR;

    Bitboard open = bitboardPaths[s][lsb(kingBB)] & blockers & ~kingBB;
    if (!open) // Does piece have line of sight to king? (bitboard is empty)
    {
        open = (GetBishopMoves(blockers, s) | GetRookMoves(blockers, s)) & bitboardRays[-dir][s];

        switch (dir)
        {
        case NORTH:
        case SOUTH:
        case EAST:
        case WEST:
            if (open & board.getBB(~color, ROOK, QUEEN) & ~sqrToBB(enPassant))
                return -dir; // return if piece also sees a rook/queen
            return NONE_DIR;
        case NORTH_EAST:
        case NORTH_WEST:
        case SOUTH_EAST:
        case SOUTH_WEST:
            if (open & board.getBB(~color, BISHOP, QUEEN) & ~sqrToBB(enPassant))
                return -dir; // return if piece also sees a bishop/queen
            return NONE_DIR;
        default:
            break;
        }
    }
    return NONE_DIR;
}

// Pawn move generation
template <MoveType mType, Color color>
void generatePawnMoves(const Board& board, MoveList* list, MovegenMasks* masks)
{
    PROFILE_FUNC();

    constexpr bool whiteToMove = color == WHITE;
    constexpr Direction forward = whiteToMove ? NORTH : SOUTH;
    constexpr Direction doubleForward = forward + forward;
    constexpr Direction forwardWest = forward + WEST;
    constexpr Direction forwardEast = forward + EAST;

    const Bitboard pawns = board.getBB(color, PAWN);
    const Bitboard empty = board.getBB(EMPTY);
    const Bitboard enemy = board.getBB(~color);

    const Bitboard pinnedPawnsS = pawns & masks->pinnedS; // these pawns can't attack but might be able to move forward
    const Bitboard pinnedPawnsD = pawns & masks->pinnedD; // these pawns can't move forward but can attack

    const Bitboard unpinnedPawnsS = pawns & ~masks->pinnedS;
    const Bitboard unpinnedPawnsD = pawns & ~masks->pinnedD;

    const Bitboard promotingPawnsMask = whiteToMove ? rankBBs[RANK_8] : rankBBs[RANK_1];

    if constexpr (mType == ALL_MOVES)
    {
        const Bitboard singlePushesUnpinned = shift(unpinnedPawnsS & ~masks->pinnedD, forward);
        const Bitboard singlePushesPinned = shift(pinnedPawnsS & ~masks->pinnedD, forward) & masks->pinnedS;
        Bitboard singlePushes =
            (singlePushesPinned | singlePushesUnpinned) & empty; // pawns can't move forward if pinned diagonally

        Bitboard doublePushes =
            shift(singlePushes, forward) & empty & (whiteToMove ? rankBBs[RANK_4] : rankBBs[RANK_5]) & masks->checkBB;
        singlePushes &= masks->checkBB;

        Bitboard promotingPawnsForward = singlePushes & promotingPawnsMask;
        singlePushes ^= promotingPawnsForward;

        while (promotingPawnsForward)
        {
            const Square to = popLSB(promotingPawnsForward);
            const Square from = to - forward;

            list->addMove(Move(from, to, PROMOTION, QUEEN));
            list->addMove(Move(from, to, PROMOTION, ROOK));
            list->addMove(Move(from, to, PROMOTION, BISHOP));
            list->addMove(Move(from, to, PROMOTION, KNIGHT));
        }

        while (doublePushes)
        {
            const Square to = popLSB(doublePushes);
            const Square from = to - doubleForward;

            list->addMove<QUIET>(from, to);
        }

        while (singlePushes)
        {
            const Square to = popLSB(singlePushes);
            const Square from = to - forward;

            list->addMove<QUIET>(from, to);
        }
    }

    const Bitboard attacksWestPinned =
        shift(pinnedPawnsD & ~fileBBs[FILE_A] & ~masks->pinnedS, forwardWest) & masks->pinnedD;
    const Bitboard attacksWestUnpinned = shift(unpinnedPawnsD & ~fileBBs[FILE_A] & ~masks->pinnedS, forwardWest);

    Bitboard attacksWest = (attacksWestPinned | attacksWestUnpinned) & enemy & masks->checkBB;

    Bitboard promotingPawnsWest = attacksWest & promotingPawnsMask;
    attacksWest ^= promotingPawnsWest;

    while (promotingPawnsWest)
    {
        const Square to = popLSB(promotingPawnsWest);
        const Square from = to - forwardWest;

        list->addMove(Move(from, to, PROMOTION, QUEEN));
        list->addMove(Move(from, to, PROMOTION, ROOK));
        list->addMove(Move(from, to, PROMOTION, BISHOP));
        list->addMove(Move(from, to, PROMOTION, KNIGHT));
    }

    while (attacksWest)
    {
        const Square to = popLSB(attacksWest);
        const Square from = to - forwardWest;

        list->addMove<CAPTURE>(from, to);
    }

    const Bitboard attacksEastPinned =
        shift(pinnedPawnsD & ~fileBBs[FILE_H] & ~masks->pinnedS, forwardEast) & masks->pinnedD;
    const Bitboard attacksEastUnpinned = shift(unpinnedPawnsD & ~fileBBs[FILE_H] & ~masks->pinnedS, forwardEast);

    Bitboard attacksEast = (attacksEastPinned | attacksEastUnpinned) & enemy & masks->checkBB;

    Bitboard promotingPawnsEast = attacksEast & promotingPawnsMask;
    attacksEast ^= promotingPawnsEast;

    while (promotingPawnsEast)
    {
        const Square to = popLSB(promotingPawnsEast);
        const Square from = to - forwardEast;

        list->addMove(Move(from, to, PROMOTION, QUEEN));
        list->addMove(Move(from, to, PROMOTION, ROOK));
        list->addMove(Move(from, to, PROMOTION, BISHOP));
        list->addMove(Move(from, to, PROMOTION, KNIGHT));
    }

    while (attacksEast)
    {
        const Square to = popLSB(attacksEast);
        const Square from = to - forwardEast;

        list->addMove<CAPTURE>(from, to);
    }

    // En Passant
    if (board.getEnPassantSqr() != SQ_NONE)
    {
        const Bitboard attacksWest = shift(pawns & ~fileBBs[FILE_A], forwardWest) & shift(masks->checkBB, forward) &
                                     sqrToBB(board.getEnPassantSqr());
        const Bitboard attacksEast = shift(pawns & ~fileBBs[FILE_H], forwardEast) & shift(masks->checkBB, forward) &
                                     sqrToBB(board.getEnPassantSqr());
        const Square enPassantAttacked = board.getEnPassantSqr() - forward;
        if (attacksEast)
        {
            const Square to = lsb(attacksEast);
            const Square from = to - forwardEast;
            const Direction pinned = isPinned<color>(board, from, enPassantAttacked);
            if (pinned == NONE_DIR || pinned == forwardEast || pinned == -forwardWest)
                list->addMove<CAPTURE>(from, to);
        }
        if (attacksWest)
        {
            const Square to = lsb(attacksWest);
            const Square from = to - (forwardWest);
            const Direction pinned = isPinned<color>(board, from, enPassantAttacked);
            if (pinned == NONE_DIR || pinned == forwardWest || pinned == -forwardEast)
                list->addMove<CAPTURE>(from, to);
        }
    }
}

// Knight move generation

template <MoveType mType, Color color>
void generateKnightMoves(const Board& board, MoveList* list, MovegenMasks* masks)
{
    PROFILE_FUNC();

    Bitboard knights =
        board.getBB(color, KNIGHT) & ~(masks->pinnedS | masks->pinnedD); // doesn't matter which way the pin is
    while (knights)
    {
        const Square from = popLSB(knights);

        const Bitboard moves = knightMoves[from] & masks->checkBB;
        Bitboard captures = moves & board.getBB(~color);
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            Bitboard quiets = moves & ~board.getBB(ALL_PIECES);
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }
}

template <MoveType mType, Color color>
void generateBishopMoves(const Board& board, MoveList* list, MovegenMasks* masks)
{
    PROFILE_FUNC();

    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard bishops = board.getBB(color, BISHOP) & ~masks->pinnedS;
    const Bitboard enemy = board.getBB(~color);

    Bitboard pinnedBishops = bishops & masks->pinnedD;
    Bitboard unpinnedBishops = bishops ^ pinnedBishops;

    while (pinnedBishops)
    {
        const Square from = popLSB(pinnedBishops);
        const Bitboard moves = GetBishopMoves(blockers, from) & masks->pinnedD & masks->checkBB;

        const Bitboard captures = moves & enemy;
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            const Bitboard quiets = moves & board.getBB(EMPTY);
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }

    while (unpinnedBishops)
    {
        const Square from = popLSB(unpinnedBishops);
        const Bitboard moves = GetBishopMoves(blockers, from) & masks->checkBB;

        const Bitboard captures = moves & enemy;
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            const Bitboard quiets = moves & board.getBB(EMPTY);
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }
}

template <MoveType mType, Color color>
void generateRookMoves(const Board& board, MoveList* list, MovegenMasks* masks)
{
    PROFILE_FUNC();
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard rooks = board.getBB(color, ROOK) & ~masks->pinnedD;
    const Bitboard enemy = board.getBB(~color);

    Bitboard pinnedRooks = rooks & masks->pinnedS;
    Bitboard unpinnedRooks = rooks ^ pinnedRooks;

    while (pinnedRooks)
    {
        const Square from = popLSB(pinnedRooks);
        const Bitboard moves = GetRookMoves(blockers, from) & masks->pinnedS & masks->checkBB;

        const Bitboard captures = moves & enemy;
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            const Bitboard quiets = moves & board.getBB(EMPTY);
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }

    while (unpinnedRooks)
    {
        const Square from = popLSB(unpinnedRooks);
        const Bitboard moves = GetRookMoves(blockers, from) & masks->checkBB;

        const Bitboard captures = moves & enemy;
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            const Bitboard quiets = moves & board.getBB(EMPTY);
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }
}

template <MoveType mType, Color color>
void generateQueenMoves(const Board& board, MoveList* list, MovegenMasks* masks)
{
    PROFILE_FUNC();

    const Bitboard blockers = board.getBB(ALL_PIECES);
    const Bitboard enemy = board.getBB(~color);
    const Bitboard empty = board.getBB(EMPTY);
    Bitboard queens = board.getBB(color, QUEEN);

    while (queens)
    {
        Bitboard moves;
        Square from = popLSB(queens);

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

        moves &= masks->checkBB;

        const Bitboard captures = moves & enemy;
        BitboardToMoves<CAPTURE>(from, captures, list);

        if constexpr (mType == ALL_MOVES)
        {
            const Bitboard quiets = moves & empty;
            BitboardToMoves<QUIET>(from, quiets, list);
        }
    }
}

template <MoveType mType, Color color>
void generateKingMoves(const Board& board, MoveList* list)
{
    PROFILE_FUNC();
    const Square from = lsb(board.getBB(color, KING));
    const Bitboard open = ~board.getAttacked(~color);

    const Bitboard enemy = board.getBB(~color);
    const Bitboard moves = kingMoves[from] & open;

    if constexpr (mType == ALL_MOVES)
    {
        Bitboard quiets = moves & board.getBB(EMPTY);
        BitboardToMoves<QUIET>(from, quiets, list);
    }

    Bitboard captures = moves & enemy;
    BitboardToMoves<CAPTURE>(from, captures, list);
}

template <Color color>
void generateCastlingMoves(const Board& board, MoveList* list)
{
    PROFILE_FUNC();
    const Bitboard blockers = board.getBB(ALL_PIECES);
    const CastlingRights castleRights = board.getState()->castling;

    const Bitboard open = ~(blockers | board.getAttacked(~color));

    if constexpr (color == WHITE)
    {
        // Short castle
        if ((castleRights & CASTLE_WK) &&                          // has castle right
            (open & castleBBs[CASTLE_WK]) == castleBBs[CASTLE_WK]) // no obstructions between king and rook
        {
            list->addMove(Move(SQ_E1, SQ_G1, CASTLE, EMPTY));
        }
        // Long castle
        if ((castleRights & CASTLE_WQ) && // has castle right
            ((open & castleBBs[CASTLE_WQ]) ==
             castleBBs[CASTLE_WQ]) &&            // no attacked squares or pieces where king moves through
            (blockers & sqrToBB(SQ_B1)) == 0ULL) // no piecce next to rook
        {
            list->addMove(Move(SQ_E1, SQ_C1, CASTLE, EMPTY));
        }
    }
    else
    {
        // Short castle
        if ((castleRights & CASTLE_BK) && ((open & castleBBs[CASTLE_BK]) == castleBBs[CASTLE_BK]))
        {
            list->addMove(Move(SQ_E8, SQ_G8, CASTLE, EMPTY));
        }
        // Long castle
        if ((castleRights & CASTLE_BQ) && ((open & castleBBs[CASTLE_BQ]) == castleBBs[CASTLE_BQ]) &&
            (blockers & sqrToBB(SQ_B8)) == 0ULL)
        {
            list->addMove(Move(SQ_E8, SQ_C8, CASTLE, EMPTY));
        }
    }
}

template <MoveType type, Color color>
void generateMoves(Board& board, MoveList* list)
{
    PROFILE_FUNC();
    assert(board.getBB(color, KING));

    const Bitboard checkers = board.getCheckers();

    if (popCount(checkers) < 2)
    {
        const Square king = lsb(board.getBB(color, KING));

        Bitboard checkBB = -1ULL;
        if (board.getNumChecks() == 1)
        {
            const Bitboard knights = checkers & board.getBB(KNIGHT);
            if (checkers & knights) // if checker is a knight
                checkBB = checkers;
            else
                checkBB = bitboardPaths[king][lsb(checkers)];
        }

        Bitboard pinnedS;
        Bitboard pinnedD;
        board.computePins(pinnedS, pinnedD);
        MovegenMasks masks = {checkBB, pinnedS, pinnedD};

        generatePawnMoves<type, color>(board, list, &masks);
        generateKnightMoves<type, color>(board, list, &masks);
        generateBishopMoves<type, color>(board, list, &masks);
        generateRookMoves<type, color>(board, list, &masks);
        generateQueenMoves<type, color>(board, list, &masks);

        if constexpr (type == ALL_MOVES)
        {
            if (checkers == 0ULL)
                generateCastlingMoves<color>(board, list);
        }
    }
    generateKingMoves<type, color>(board, list);
}

template void generateMoves<ALL_MOVES, WHITE>(Board& board, MoveList* list);
template void generateMoves<ALL_MOVES, BLACK>(Board& board, MoveList* list);

template void generateMoves<CAPTURE, WHITE>(Board& board, MoveList* list);
template void generateMoves<CAPTURE, BLACK>(Board& board, MoveList* list);