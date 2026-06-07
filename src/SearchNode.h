#ifndef SEARCH_NODE_H
#define SEARCH_NODE_H

#include "move.h"
#include "nnue/accumulator.h"
#include <cstdint>

struct PVLine
{
    uint8_t len;
    Move moves[MAX_DEPTH];

    PVLine() : len(0)
    {
    }
};

struct SearchNode
{
    SearchNode()
    {
        staticEval = 0;
        pvLine.len = 0;
    }

    SearchNode(SearchNode* parent) : prev(parent)
    {
        staticEval = 0;
        pvLine.len = 0;
    }

    void ComputeAccumulator(const Board& board);

    PVLine pvLine;
    Score staticEval;

    SearchNode* prev;
};

#endif