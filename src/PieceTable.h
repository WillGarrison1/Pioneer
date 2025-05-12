#ifndef PSQT_H
#define PSQT_H

#include "evaluate.h"
#include "square.h"

// clang-format off

constexpr Score PawnPSQ[] = { 
     0,  0,  0,  0,  0,  0,  0,  0,  // 1
    50, 50, 50, 50, 50, 50, 50, 50,  // 2
    10, 10, 20, 30, 30, 20, 10, 10,  // 3
     5,  5, 10, 25, 25, 10,  5,  5,  // 4
     0,  0,  0, 20, 20,  0,  0,  0,  // 5
     5, -5,-10,  0,  0,-10, -5,  5,  // 6
     5, 10, 10,-30,-30, 10, 10,  5,  // 7
     0,  0,  0,  0,  0,  0,  0,  0}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score KnightPSQ[] = { 
   -50,-40,-30,-30,-30,-30,-40,-50,  // 1
   -40,-20,  0,  0,  0,  0,-20,-40,  // 2
   -30,  0, 10, 15, 15, 10,  0,-30,  // 3
   -30,  5, 15, 20, 20, 15,  5,-30,  // 4
   -30,  0, 15, 20, 20, 15,  0,-30,  // 5
   -30,  5, 10, 15, 15, 10,  5,-30,  // 6
   -40,-20,  0,  5,  5,  0,-20,-40,  // 7
   -50,-35,-30,-30,-30,-30,-35,-50}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score BishopPSQ[] = { 
   -20,-10,-10,-10,-10,-10,-10,-20,  // 1
   -10,  0,  0,  0,  0,  0,  0,-10,  // 2
   -10,  0,  5, 10, 10,  5,  0,-10,  // 3
   -10,  5,  5, 10, 10,  5,  5,-10,  // 4
   -10,  0, 10, 10, 10, 10,  0,-10,  // 5
   -10, 10, 10, 10, 10, 10, 10,-10,  // 6
   -10,  5,  0,  0,  0,  0,  5,-10,  // 7
   -20,-10,-10,-10,-10,-10,-10,-20}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score RookPSQ[] = { 
     0,  0,  0,  0,  0,  0,  0,  0,  // 1
     5, 10, 10, 10, 10, 10, 10,  5,  // 2
    -5,  0,  0,  0,  0,  0,  0, -5,  // 3
    -5,  0,  0,  0,  0,  0,  0, -5,  // 4
    -5,  0,  0,  0,  0,  0,  0, -5,  // 5
    -5,  0,  0,  0,  0,  0,  0, -5,  // 6
    -5,  0,  0,  0,  0,  0,  0, -5,  // 7
     0,  0,  0,  5,  5,  0,  0,  0}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score QueenPSQ[] = { 
    -20,-10,-10, -5, -5,-10,-10,-20,  // 1
    -10,  0,  0,  0,  0,  0,  0,-10,  // 2
    -10,  0,  5,  5,  5,  5,  0,-10,  // 3
     -5,  0,  5,  5,  5,  5,  0, -5,  // 4
      0,  0,  5,  5,  5,  5,  0, -5,  // 5
    -10,  5,  5,  5,  5,  5,  0,-10,  // 6
    -10,  0,  5,  0,  0,  0,  0,-10,  // 7
    -20,-10,-10, -5, -5,-10,-10,-20}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score KingPSQ_MG[] = { 
    -30,-40,-40,-50,-50,-40,-40,-30,  // 1
    -30,-40,-40,-50,-50,-40,-40,-30,  // 2
    -30,-40,-40,-50,-50,-40,-40,-30,  // 3
    -30,-40,-40,-50,-50,-40,-40,-30,  // 4
    -20,-30,-30,-40,-40,-30,-30,-20,  // 5
    -10,-20,-20,-20,-20,-20,-20,-10,  // 6
     20, 20,  0,  0,  0,  0, 20, 20,  // 7
     20, 30, 10,  0,  0, 10, 30, 20}; // 8
//   H   G   F   E   D   C   B   A

constexpr Score KingPSQ_EG[] = { 
    -50,-40,-30,-20,-20,-30,-40,-50,  // 1
    -30,-20,-10,  0,  0,-10,-20,-30,  // 2
    -30,-10, 20, 30, 30, 20,-10,-30,  // 3
    -30,-10, 30, 40, 40, 30,-10,-30,  // 4
    -30,-10, 30, 40, 40, 30,-10,-30,  // 5
    -30,-10, 20, 30, 30, 20,-10,-30,  // 6
    -30,-30,  0,  0,  0,  0,-30,-30,  // 7
    -50,-30,-30,-30,-30,-30,-30,-50}; // 8
//   H   G   F   E   D   C   B   A
// clang-format on

template <PieceType P, Color S>
constexpr Score GetPSQValue(const Square s)
{
    if constexpr (S == WHITE)
    {
        if constexpr (P == PAWN)
            return PawnPSQ[s ^ 56];

        if constexpr (P == KNIGHT)
            return KnightPSQ[s ^ 56];

        if constexpr (P == BISHOP)
            return BishopPSQ[s ^ 56];

        if constexpr (P == ROOK)
            return RookPSQ[s ^ 56];

        if constexpr (P == QUEEN)
            return QueenPSQ[s ^ 56];

        if constexpr (P == KING)
            return KingPSQ_MG[s ^ 56];
    }
    else
    {
        if constexpr (P == PAWN)
            return PawnPSQ[s];

        if constexpr (P == KNIGHT)
            return KnightPSQ[s];

        if constexpr (P == BISHOP)
            return BishopPSQ[s];

        if constexpr (P == ROOK)
            return RookPSQ[s];

        if constexpr (P == QUEEN)
            return QueenPSQ[s];

        if constexpr (P == KING)
            return KingPSQ_MG[s];
    }
}

template <Color S>
constexpr Score GetPSQValue(const PieceType P, const Square s)
{
    if constexpr (S == WHITE)
    {
        switch (P)
        {
        case PAWN:
            return PawnPSQ[s ^ 56];
        case KNIGHT:
            return KnightPSQ[s ^ 56];
        case BISHOP:
            return BishopPSQ[s ^ 56];
        case ROOK:
            return RookPSQ[s ^ 56];
        case QUEEN:
            return QueenPSQ[s ^ 56];
        case KING:
            return KingPSQ_MG[s ^ 56];
        default:
            return 0;
        }
    }
    else
    {
        switch (P)
        {
        case PAWN:
            return PawnPSQ[s];
        case KNIGHT:
            return KnightPSQ[s];
        case BISHOP:
            return BishopPSQ[s];
        case ROOK:
            return RookPSQ[s];
        case QUEEN:
            return QueenPSQ[s];
        case KING:
            return KingPSQ_MG[s];
        default:
            return 0;
        }
    }
}

#endif