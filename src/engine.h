#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>

#include "board.h"
#include "search.h"

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
        board->getFen();
    }

    void setFen(const std::string& fen)
    {
        board->setFen(fen, &states[0]);
    }

    void go(unsigned int depth, unsigned int nodes, unsigned int movetime, unsigned int wtime, unsigned int btime);
    void goPerft(unsigned int depth);

    void stop();

    void eval();

    void makemove(Move move);
    void undomove()
    {
        board->undoMove();
    }

    void isCheck(Move move);
    void ClearTT()
    {
        searcher->ClearTT();
    }

  private:
    Board* board;
    Searcher* searcher;
    BoardState states[MAX_PLY + 1]; // plus 1 for starting point
};

#endif