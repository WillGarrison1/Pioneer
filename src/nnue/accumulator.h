#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "../move.h"
#include <cstdint>

struct DirtyMove
{
    Piece movePiece;
    Piece promote;
    Piece capturedPiece;

    Square from;
    Square to;
    Square captured;

    Square castleFrom;
    Square castleTo;
};

struct Accumulator
{
    alignas(64) int16_t data[1024];
    alignas(64) int32_t psqt[8];
};

struct AccumulatorNode
{
    Accumulator whiteAcc, blackAcc;

    bool isWhiteComputed, isBlackComputed;
    DirtyMove dirtyMove;

    AccumulatorNode()
    {
        isBlackComputed = isWhiteComputed = false;
        dirtyMove.movePiece = EMPTY;
    }
};

#endif