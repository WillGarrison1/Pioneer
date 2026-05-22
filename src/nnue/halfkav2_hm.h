#ifndef HALFKAV2_HM_H
#define HALFKAV2_HM_H

#include "../types.h"
#include "../square.h"
#include <cassert>

#define NUM_FEATURES 22528

constexpr int GetIndex(Square square, Square king_square, const PieceType piece, const bool whitePOV,
                             const bool isBlackPiece)
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

    uint16_t output = (king_square * 11 * 64) +
                      (static_cast<uint32_t>(piece) * 2 + ((piece + 1 == KING) ? 0 : (isBlackPiece == whitePOV))) * 64 +
                      square;

    assert(output < NUM_FEATURES);

    return output;
}
#endif