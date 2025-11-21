#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"

const Score pieceScores[] = {0, 100, 320, 330, 500, 900, 0};

enum EvalType
{
    FAST,
    FULL
};

template <EvalType type>
Score Eval(Board &board);

#endif