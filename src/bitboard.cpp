#include <cstring>
#include <iostream>

#include "bitboard.h"
#include "direction.h"
#include "square.h"

Bitboard knightMoves[64];
Bitboard kingMoves[64];
Bitboard pawnAttacks[9][64];
Bitboard pawnMoves[9][64];
Bitboard rookMasks[64];
Bitboard bishopMasks[64];
Bitboard bitboardPaths[64][64]; // bitboards with bits from one square to another (note that the start squares/first
                                // index isn't included)
Bitboard _bitboardRays[19][64] =
    {}; // bitboards with bits starting from a certain square heading off the board (start square isn't included)
const Bitboard (*bitboardRays)[64] = _bitboardRays + 9;

Bitboard passedPawnBB[9][64];
Bitboard isolatedPawnBB[64];

void initBBs()
{

    constexpr Direction knightDirs[] = {NORTH + NORTH + EAST, NORTH + NORTH + WEST, SOUTH + SOUTH + EAST,
                                        SOUTH + SOUTH + WEST, EAST + EAST + NORTH,  EAST + EAST + SOUTH,
                                        WEST + WEST + NORTH,  WEST + WEST + SOUTH};

    constexpr Direction kingDirs[] = {NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST};

    constexpr Direction pawnWAttackDirs[] = {NORTH_EAST, NORTH_WEST};
    constexpr Direction pawnBAttackDirs[] = {SOUTH_EAST, SOUTH_WEST};

    memset(_bitboardRays, 0, sizeof(_bitboardRays));

    // Initialize moves and attacks
    for (Square sqr = SQ_A1; sqr <= SQ_H8; sqr++)
    {
        for (Direction dir : knightDirs)
        {
            Square to = sqr + dir;
            if (to >= 0 && to < 64 && manhattanDistance(sqr, to) == 3)
                setBit(knightMoves[sqr], to);
        }

        for (Direction dir : kingDirs)
        {
            Square to = sqr + dir;
            if (to >= 0 && to < 64 && manhattanDistance(sqr, to) == 1)
                setBit(kingMoves[sqr], to);
            if (to >= 0 && to < 64 && manhattanDistance(sqr, to) == 2)
                setBit(kingMoves[sqr], to);
        }

        for (Direction dir : pawnWAttackDirs)
        {
            Square to = sqr + dir;
            if (to >= 0 && to < 64 && manhattanDistance(sqr, to) == 2)
                setBit(pawnAttacks[WHITE][sqr], to);
        }

        for (Direction dir : pawnBAttackDirs)
        {
            Square to = sqr + dir;
            if (to >= 0 && to < 64 && manhattanDistance(sqr, to) == 2)
                setBit(pawnAttacks[BLACK][sqr], to);
        }

        // Pawn moves
        Square to = sqr + NORTH;
        if (to >= 0 && to < 64)
            setBit(pawnMoves[WHITE][sqr], to);

        to = sqr + SOUTH;
        if (to >= 0 && to < 64)
            setBit(pawnMoves[BLACK][sqr], to);

        // Double pawn moves
        if (getRank(sqr) == 1)
        {
            to = sqr + NORTH + NORTH;
            if (to >= 0 && to < 64)
                setBit(pawnMoves[WHITE][sqr], to);
        }

        if (getRank(sqr) == 6)
        {
            to = sqr + SOUTH + SOUTH;
            if (to >= 0 && to < 64)
                setBit(pawnMoves[BLACK][sqr], to);
        }

        // Bitboard rays
        for (Direction dir = SOUTH_WEST; dir <= NORTH_EAST; dir++)
        {
            Square i = sqr;
            while (dir && distToEdge[dir][i] != 0)
            {
                i += dir;
                setBit(_bitboardRays[dir + 9][sqr], i);
            }
        }

        for (Square sqr2 = SQ_A1; sqr2 <= SQ_H8; sqr2++)
        {
            Direction dir = directionsTable[sqr][sqr2];
            if (dir != NONE_DIR)
            {
                Square i = sqr;
                while (i != sqr2)
                {
                    i += dir;
                    setBit(bitboardPaths[sqr][sqr2], i);
                }
            }
        }

        rookMasks[sqr] =
            bitboardRays[NORTH][sqr] | bitboardRays[SOUTH][sqr] | bitboardRays[EAST][sqr] | bitboardRays[WEST][sqr];
        bishopMasks[sqr] = bitboardRays[SOUTH_WEST][sqr] | bitboardRays[SOUTH_EAST][sqr] |
                           bitboardRays[NORTH_EAST][sqr] | bitboardRays[NORTH_WEST][sqr];

        // Passed pawn detection
        Rank rank = getRank(sqr);
        File file = getFile(sqr);
        File left, right;
        left = right = file;
        if (file != FILE_A)
            left = file - 1;
        if (file != FILE_H)
            right = file + 1;
        if (rank != RANK_1 && rank != RANK_8)
        {
            passedPawnBB[WHITE][sqr] = shift(fileBBs[file] | fileBBs[left] | fileBBs[right], NORTH * (rank + 1));
            passedPawnBB[BLACK][sqr] = shift(fileBBs[file] | fileBBs[left] | fileBBs[right], SOUTH * (8 - rank));
        }
        isolatedPawnBB[sqr] = (fileBBs[left] | fileBBs[right]) & ~fileBBs[file];
    }
}

Bitboard sendRay(Square i, const Direction dir, const Bitboard blockers)
{

    if (!distToEdge[dir][i])
        return 0ULL;
    if (!(bitboardRays[dir][i] & blockers))
        return bitboardRays[dir][i];

    Bitboard bb = 0ULL;

    do
    {
        i += dir;
        setBit(bb, i);
    } while (distToEdge[dir][i] && !getBit(blockers, i));

    return bb;
}