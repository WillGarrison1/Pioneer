#ifndef MAGIC_H
#define MAGIC_H

#include "types.h"

struct Magic
{
    Bitboard* moves;
    Key magic;
    unsigned char offset;
};

Magic rookMagics[64];
Magic bishopMagics[64];

#endif