#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "SearchNode.h"

constexpr Score pieceScores[] = {0, 100, 320, 330, 500, 900, 0};

enum EvalType
{
    FAST,
    FULL
};

template <EvalType type>
Score Eval(Board& board, SearchNode* node);


#endif