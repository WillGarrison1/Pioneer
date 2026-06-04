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
        accumulatorNode.isBlackComputed = accumulatorNode.isWhiteComputed = false;
        accumulatorNode.dirtyMove.movePiece = EMPTY;
    }

    SearchNode(SearchNode* parent) : prev(parent)
    {
        staticEval = 0;
        pvLine.len = 0;
        accumulatorNode.isBlackComputed = accumulatorNode.isWhiteComputed = false;
        accumulatorNode.dirtyMove.movePiece = EMPTY;
    }

    void ComputeAccumulator(const Board& board);

    PVLine pvLine;
    AccumulatorNode accumulatorNode;
    Score staticEval;

    SearchNode* prev;
};

#endif