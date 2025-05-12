#ifndef TYPES_H
#define TYPES_H

enum Color : unsigned int
{
    WHITE,
    BLACK = 8
};

enum PieceType : unsigned int
{
    EMPTY = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    ALL_PIECES = 7
};

// clang-format off
enum Square : unsigned int
{
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,

    SQ_NONE = 64
};
// clang-format on

enum File : int
{
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H
};

enum Rank : int
{
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8
};

constexpr File operator-(File file, File file2)
{
    return static_cast<File>(static_cast<int>(file) - static_cast<int>(file2));
}

constexpr File operator-(File file, int file2)
{
    return static_cast<File>(static_cast<int>(file) - file2);
}

constexpr File operator+(File file, File file2)
{
    return static_cast<File>(static_cast<int>(file) + static_cast<int>(file2));
}

constexpr File operator+(File file, int file2)
{
    return static_cast<File>(static_cast<int>(file) + file2);
}

constexpr File operator++(File& file, const int)
{
    File temp = file;
    file = file + 1;
    return temp;
}

enum Direction : int
{
    NONE_DIR = 0,
    NORTH = 8,
    SOUTH = -8,
    EAST = 1,
    WEST = -1,
    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

enum CastlingRights : unsigned int
{
    NONE_CASTLE = 0,
    CASTLE_WK = 1,
    CASTLE_WQ = 2,
    CASTLE_BK = 4,
    CASTLE_BQ = 8
};

constexpr CastlingRights operator|(CastlingRights a, CastlingRights b)
{
    return static_cast<CastlingRights>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr CastlingRights operator&(CastlingRights a, CastlingRights b)
{
    return static_cast<CastlingRights>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr CastlingRights operator~(CastlingRights a)
{
    return static_cast<CastlingRights>(~static_cast<int>(a));
}

inline void operator&=(CastlingRights& a, CastlingRights b)
{
    a = a & b;
}
inline void operator|=(CastlingRights& a, CastlingRights b)
{
    a = a | b;
}

enum MoveType : unsigned int
{
    QUIET = 0,
    CAPTURE = 1,
    CASTLE = 2,
    EN_PASSANT = 4,
    PROMOTION = 8,
    ALL_MOVES = 31
};

constexpr MoveType operator|(MoveType a, MoveType b)
{
    return static_cast<MoveType>(static_cast<int>(a) | static_cast<int>(b));
}
// typedefs

typedef unsigned int Piece;
typedef unsigned long long Bitboard;
typedef int Score;

// Forward declarations

struct MoveList;

class Move;
class Engine;
class Board;

#endif // TYPES_H