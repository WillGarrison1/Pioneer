#ifndef HALFKAV2_HM_H
#define HALFKAV2_HM_H

#include "../square.h"
#include "../types.h"
#include "../color.h"
#include "../piece.h"
#include <cassert>

#define NUM_FEATURES 22528

constexpr int GetIndex(Square square, Square king_square, const Piece piece, const bool whitePOV)
{
    if (getFile(king_square) > FILE_D)
    {
        File kf = getFile(king_square);
        king_square = getSquare(FILE_H - kf, getRank(king_square));

        File f = getFile(square);
        square = getSquare(FILE_H - f, getRank(square));
    }

    if (!whitePOV)
    {
        square ^= 56;
        king_square ^= 56;
    }

    king_square -= int(king_square / 8) * 4;

    int kingBucket = king_square * 11 * 64;

    bool isBlackPiece = getColor(piece) == BLACK;
    PieceType pType = getType(piece);
    bool isEnemy = isBlackPiece == whitePOV && pType != KING;

    uint16_t output = kingBucket + ((pType - 1) * 2 + isEnemy) * 64 + square;

    assert(output < NUM_FEATURES);

    return output;
}
#endif