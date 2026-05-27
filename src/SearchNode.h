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
    SearchNode() = default;
    SearchNode(SearchNode* parent) : prev(parent)
    {
        accumulatorNode.isBlackComputed = accumulatorNode.isWhiteComputed = false;
    }

    void ComputeAccumulator(const Board& board);

    PVLine pvLine;
    AccumulatorNode accumulatorNode;

    SearchNode* prev;
};

#endif