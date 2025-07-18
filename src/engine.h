#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"

/**
 * @brief Chess engine class
 */
class Engine
{
  public:
    Engine();
    ~Engine();

    void print()
    {
        board->print();
    }

    void setFen(const std::string& fen)
    {
        board->setFen(fen);
    }

    void go(unsigned int depth, unsigned int nodes, unsigned int movetime);
    void goPerft(unsigned int depth);

    void eval();

    void makemove(Move move);
    void undomove()
    {
        board->undoMove();
    }

    void isCheck(Move move);

  private:
    Board* board;
};

#endif