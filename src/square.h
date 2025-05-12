#ifndef SQUARE_H
#define SQUARE_H

#include "types.h"
#include <cmath>

// Gives the distance (no diagnals) to the edge of the board from a certain square
// Where the the closest square to the edge is 0

extern const int (*distToEdge)[64];

constexpr Rank getRank(Square sq)
{
    return static_cast<Rank>(sq >> 3);
}

constexpr File getFile(Square sq)
{
    return static_cast<File>(sq & 7);
}

constexpr Square getSquare(File file, Rank rank)
{
    return static_cast<Square>(rank * 8 + file);
}

// Square operators

constexpr Square operator+(const Square sq, const Square sq2)
{
    return static_cast<Square>(static_cast<int>(sq) + static_cast<int>(sq2));
}

constexpr Square operator+(const Square sq, const Direction dir)
{
    return static_cast<Square>(static_cast<int>(sq) + static_cast<int>(dir));
}

constexpr Square operator-(const Square sq, const Direction dir)
{
    return static_cast<Square>(static_cast<int>(sq) - static_cast<int>(dir));
}

constexpr Square operator+(const Square sq, const int offset)
{
    return static_cast<Square>(static_cast<int>(sq) + offset);
}

constexpr Square operator-(const Square sq, const int offset)
{
    return static_cast<Square>(static_cast<int>(sq) - offset);
}

constexpr Square operator+=(Square& sq, const int offset)
{
    sq = sq + offset;
    return sq;
}

constexpr Square operator-=(Square& sq, const int offset)
{
    sq = sq - offset;
    return sq;
}

constexpr Square operator++(Square& sq, const int)
{
    Square temp = sq;
    sq = sq + 1;
    return temp;
}

constexpr Square operator--(Square& sq, const int)
{
    Square temp = sq;
    sq = sq - 1;
    return temp;
}

constexpr Square operator>>(const Square sq, const int amt)
{
    return static_cast<Square>(static_cast<int>(sq) >> amt);
}

constexpr unsigned int manhattanDistance(const Square a, const Square b)
{
    return std::abs(getRank(a) - getRank(b)) + std::abs(getFile(a) - getFile(b));
}

extern void initSquare();

#endif // SQUARE_H