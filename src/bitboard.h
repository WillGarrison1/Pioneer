#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"
#include <cassert>

const Bitboard emptyBB = 0ULL;
const Bitboard fullBB = 0xFFFFFFFFFFFFFFFFULL;

// clang-format off
alignas(64) const Bitboard rankBBs[] =
    {
        0xFFULL,
        0xFF00ULL,
        0xFF0000ULL,
        0xFF000000ULL,
        0xFF00000000ULL,
        0xFF0000000000ULL,
        0xFF000000000000ULL,
        0xFF00000000000000ULL};

alignas(64) const Bitboard fileBBs[] =
    {
        0x0101010101010101ULL,
        0x0202020202020202ULL,
        0x0404040404040404ULL,
        0x0808080808080808ULL,
        0x1010101010101010ULL,
        0x2020202020202020ULL,
        0x4040404040404040ULL,
        0x8080808080808080ULL};

// Used for finding enemy attacks between castles
alignas(64) const Bitboard castleBBs[] = {
    0x0000000000000000ULL,
    0x0000000000000060ULL, // Short white
    0x000000000000000CULL, // Long white
    0x0000000000000000ULL,
    0x6000000000000000ULL, // Short black
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000000ULL,
    0x0C00000000000000ULL, // Long black
};

// clang-format on
// Move generation

extern Bitboard knightMoves[64];
extern Bitboard kingMoves[64];
extern Bitboard pawnAttacks[9][64];
extern Bitboard pawnMoves[9][64];
extern Bitboard rookMasks[64];
extern Bitboard bishopMasks[64];
extern Bitboard bitboardPaths[64][64];

extern const Bitboard (*bitboardRays)[64];

// Evaluation

extern Bitboard passedPawnBB[9][64]; // contains masks for finding passed pawns [side][square]
extern Bitboard isolatedPawnBB[64];  // contains masks for finding isloated pawns [square]

// Bitboard operations

constexpr Bitboard sqrToBB(const Square sq)
{
    return 1ULL << sq;
}

inline void setBit(Bitboard &bb, const Square sq)
{
    bb |= 1ULL << sq;
}

inline void clearBit(Bitboard &bb, const Square sq)
{
    bb &= ~(1ULL << sq);
}

inline void toggleBit(Bitboard &bb, const Square sq)
{
    bb ^= 1ULL << sq;
}

inline bool getBit(const Bitboard bb, const Square sq)
{
    return bb & (1ULL << sq);
}

constexpr Square lsb(const Bitboard bb)
{
    assert(bb != 0);
#if _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, bb);
    return static_cast<Square>(index);
#else
    return static_cast<Square>(__builtin_ctzll(bb));
#endif
}

constexpr Square msb(const Bitboard bb)
{
    assert(bb != 0);
#if _MSC_VER
    unsigned long index;
    _BitScanReverse64(&index, bb);
    return static_cast<Square>(index);
#else
    return static_cast<Square>(__builtin_clzll(bb));
#endif
}

inline Square popLSB(Bitboard &bb)
{
    Square bit = lsb(bb);
    bb &= bb - 1;
    return bit;
}

constexpr int popCount(const Bitboard bb)
{
#if _MSC_VER
    return __popcnt64(bb);
#else
    return __builtin_popcountll(bb);
#endif
}

constexpr Bitboard operator>>(const Bitboard b, const Direction d)
{
    return (static_cast<unsigned long long>(b) >> static_cast<unsigned long long>(d));
}

extern Bitboard sendRay(Square i, const Direction dir, const Bitboard blockers);
extern void initBBs();

#endif