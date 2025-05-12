#ifndef PIECE_H
#define PIECE_H

#include "types.h"

#include <string>

constexpr PieceType getType(Piece piece)
{
    return static_cast<PieceType>(piece & 0x7);
}

constexpr Color getColor(Piece piece)
{
    return static_cast<Color>(piece & 8);
}

constexpr Piece makePiece(PieceType piece, Color color)
{
    return static_cast<Piece>(piece | color);
}

// String functions
extern std::string pieceToString(Piece piece);
extern Piece stringToPiece(const std::string& str);

#endif