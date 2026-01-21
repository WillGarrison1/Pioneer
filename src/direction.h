#ifndef DIRECTION_H
#define DIRECTION_H

#include "types.h"
#include <cmath>

// Precompute stuff

extern Direction directionsTable[64][64];

// Buncha operators

constexpr Direction operator+(Direction a, Direction b)
{
    return static_cast<Direction>(static_cast<int>(a) + static_cast<int>(b));
}

constexpr Direction operator-(Direction a, Direction b)
{
    return static_cast<Direction>(static_cast<int>(a) - static_cast<int>(b));
}

constexpr Direction operator+(Direction a, int b)
{
    return static_cast<Direction>(static_cast<int>(a) + b);
}

constexpr Direction operator-(Direction a, int b)
{
    return static_cast<Direction>(static_cast<int>(a) - b);
}

constexpr Direction operator*(Direction a, int b)
{
    return static_cast<Direction>(static_cast<int>(a) * b);
}

constexpr Direction operator*(Direction a, File b)
{
    return static_cast<Direction>(static_cast<int>(a) * static_cast<int>(b));
}

constexpr Direction operator<<(const Direction a, int b)
{
    return static_cast<Direction>((static_cast<uint32_t>(std::abs(a)) << b) * a / std::abs(a));
}

constexpr Direction operator>>(const Direction a, int b)
{
    return static_cast<Direction>(static_cast<int>(a) >> b);
}

constexpr Direction& operator+=(Direction& a, Direction b)
{
    a = a + b;
    return a;
}

constexpr Direction& operator-=(Direction& a, Direction b)
{
    a = a - b;
    return a;
}

constexpr Direction& operator+=(Direction& a, int b)
{
    a = a + b;
    return a;
}

constexpr Direction& operator-=(Direction& a, int b)
{
    a = a - b;
    return a;
}

constexpr Direction& operator*=(Direction& a, int b)
{
    a = a * b;
    return a;
}

constexpr Direction operator-(Direction a)
{
    return static_cast<Direction>(-static_cast<int>(a));
}

constexpr Direction& operator++(Direction& a)
{
    a = a + 1;
    return a;
}

constexpr Direction& operator--(Direction& a)
{
    a = a - 1;
    return a;
}

constexpr Direction operator++(Direction& a, int)
{
    Direction temp = a;
    a = a + 1;
    return temp;
}

constexpr Bitboard shift(Bitboard d, Direction dir)
{
    if (dir < 0)
        return d >> -dir;
    else
        return d << dir;
}

extern void initDirection();

#endif