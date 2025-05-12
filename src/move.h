#ifndef MOVE_H
#define MOVE_H

#include "types.h"

#include <cassert>
#include <string>

/**
 * @brief Represents a move, it contains 32bits that stores the move
 * @paragraph
 * The format is as follows:
 *  (Bit)    (Description)
 *  1 - 6  : From square
 *  7 - 12 : To square
 * 13 - 16 : Move piece
 * 17 - 20 : Captured piece
 * 21 - 26 : Move type
 * 27 - 29 : Castle type
 * 30 - 32 : Promotion PieceType
 */
class Move
{
  public:
    constexpr Move() : m_move(0)
    {
    }
    constexpr Move(Square from, Square to, Piece piece, Piece captured = 0, MoveType type = QUIET,
                   CastlingRights cRights = NONE_CASTLE, PieceType promotion = EMPTY)
        : m_move((from) | (to << 6) | (piece << 12) | (captured << 16) | (type << 20) | (cRights << 25) |
                 (promotion << 29))
    {
    }

    Move(unsigned int move) : m_move(move)
    {
    }
    Move(const Move& move) : m_move(move.m_move)
    {
    }
    Move(const std::string str);

    constexpr Square from() const
    {
        return static_cast<Square>(m_move & 0x3F);
    }

    constexpr Square to() const
    {
        return static_cast<Square>((m_move >> 6) & 0x3F);
    }

    constexpr Piece piece() const
    {
        return static_cast<Piece>((m_move >> 12) & 0xF);
    }

    constexpr Piece captured() const
    {
        return static_cast<Piece>((m_move >> 16) & 0xF);
    }

    constexpr CastlingRights castleRights() const
    {
        return static_cast<CastlingRights>((m_move >> 25) & 0xF);
    }

    constexpr MoveType type() const
    {
        return static_cast<MoveType>((m_move >> 20) & 0xF);
    }

    constexpr PieceType promotion() const
    {
        return static_cast<PieceType>((m_move >> 29) & 0x7);
    }

    constexpr unsigned int getMove() const
    {
        return m_move;
    }
    
    std::string toString();


  private:
    unsigned int m_move;
};

struct MoveList
{
    Move moves[256];
    int size = 0;

    void addMove(Move move)
    {
        assert(size < 256);
        moves[size++] = move;
    }

    void clear()
    {
        size = 0;
    }

    Move operator[](int index) const
    {
        assert(index < size);
        return moves[index];
    }

    Move& operator[](int index)
    {
        assert(index < size);
        return moves[index];
    }
};

#endif