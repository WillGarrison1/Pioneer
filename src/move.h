#ifndef MOVE_H
#define MOVE_H

#include "types.h"

#include <cassert>
#include <string>

/**
 * @brief Represents a move, it contains 16bits that stores the move
 * @paragraph
 * The format is as follows:
 *  (Bit)    (Description)
 *  1 - 6  : From square
 *  7 - 12 : To square
 * 13 - 14 : Move type (Quiet, Capture, Castle, Promotion)
 * 15 - 16 : Promotion PieceType (K,B,R,Q)
 */
class Move
{
  public:
    constexpr Move() : m_move(0)
    {
    }
    constexpr Move(const Square from, const Square to, const MoveType type = QUIET, const PieceType promotion = KNIGHT)
        : m_move((from) | (to << 6) | (type << 12) | ((promotion - 2) << 14))
    {
    }

    constexpr Move(unsigned short move) : m_move(move)
    {
    }
    constexpr Move(const Move& move) : m_move(move.m_move)
    {
    }
    Move(const std::string str);

    constexpr Move& operator=(const Move& other)
    {
        m_move = other.m_move;
        return *this;
    }

    constexpr Square from() const
    {
        return static_cast<Square>(m_move & 0x3F);
    }

    constexpr Square to() const
    {
        return static_cast<Square>((m_move >> 6) & 0x3F);
    }

    constexpr MoveType type() const
    {
        return static_cast<MoveType>((m_move >> 12) & 0x3);
    }

    constexpr PieceType promotion() const
    {
        return static_cast<PieceType>(((m_move >> 14) & 0x3) + 2);
    }

    // Tells if a move is of a type
    template <MoveType T>
    constexpr bool isType() const
    {
        return type() == T;
    }

    constexpr unsigned short getMove() const
    {
        return m_move;
    }

    std::string toString();

  private:
    unsigned short m_move;
};

constexpr bool operator==(Move m1, Move m2)
{
    return m1.getMove() == m2.getMove();
}

constexpr bool operator!=(Move m1, Move m2)
{
    return m1.getMove() != m2.getMove();
}

struct MoveList
{
    Move moves[256];
    Move* end = moves;

    void addMove(Move move)
    {
        assert(GetSize() < 256);
        *end++ = move;
    }

    template <MoveType mType>
    void addMove(const Square& from, const Square& to)
    {
        assert(GetSize() < 256);
        *end++ = {from, to, mType};
    }

    void clear()
    {
        end = moves;
    }

    Move operator[](int index) const
    {
        assert(index < GetSize());
        return moves[index];
    }

    Move& operator[](int index)
    {
        assert(index < GetSize());
        return moves[index];
    }

    inline int GetSize() const
    {
        return end - moves;
    }
};

#endif