#ifndef MAGIC_H
#define MAGIC_H

#include "bitboard.h"
#include "types.h"

struct Magic
{
    Bitboard* moves;
    Key magic;
    unsigned char offset;

    Magic() : moves(nullptr), magic(0), offset(0)
    {
    }

    Magic(Key magicULL, unsigned char offset) : moves(nullptr), magic(magicULL), offset(offset)
    {
    }

    inline Bitboard GetMoves(Bitboard blockers)
    {
        return moves[(magic * blockers) >> (64 - offset)];
    }
};

extern Bitboard noEdgeMask[64];

extern Magic rookMagics[64];
extern Magic bishopMagics[64];

void InitMagics();

inline Bitboard GetRookMoves(Bitboard blockers, Square sqr)
{
    return rookMagics[sqr].GetMoves(blockers & rookMasks[sqr] & noEdgeMask[sqr] & ~sqrToBB(sqr));
}

inline Bitboard GetBishopMoves(Bitboard blockers, Square sqr)
{
    return bishopMagics[sqr].GetMoves(blockers & bishopMasks[sqr] & noEdgeMask[sqr] & ~sqrToBB(sqr));
}

#endif