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
    int16_t data[512];
    int32_t psqt[8];

    void Add(Square square, Square kingSquare, Piece piece, bool whitePOV);    // adds features
    void Remove(Square square, Square kingSquare, Piece piece, bool whitePOV); // removes features
    void Update(Square from, Square to, Square kingSquare, Piece fromPiece, Piece toPiece,
                bool whitePOV); // substitutes features
};

struct AccumulatorNode
{
    Accumulator whiteAcc, blackAcc;

    bool isWhiteComputed, isBlackComputed;
    DirtyMove dirtyMove;
};

#endif