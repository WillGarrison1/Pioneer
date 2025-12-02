#ifndef MAGIC_H
#define MAGIC_H

#include "bitboard.h"
#include "types.h"
#include <immintrin.h>

struct Magic
{
    Bitboard* moves;
    Bitboard mask;

    Magic() : moves(nullptr), mask(0)
    {
    }

    Magic(Bitboard mask) : moves(nullptr), mask(mask)
    {
    }

    inline Bitboard GetMoves(const Bitboard blockers)
    {
        return moves[_pext_u64(blockers, mask)];
    }
};

extern Magic rookMagics[64];
extern Magic bishopMagics[64];

void InitMagics();

inline Bitboard GetRookMoves(const Bitboard blockers, const Square sqr)
{
    return rookMagics[sqr].GetMoves(blockers);
}

inline Bitboard GetBishopMoves(const Bitboard blockers, const Square sqr)
{
    return bishopMagics[sqr].GetMoves(blockers);
}

#endif