#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>

#include "board.h"
#include "search.h"

/**
 * @brief Raw `go` arguments as sent by the GUI, before they are resolved for the side to move.
 */
struct GoParams
{
    unsigned int depth = 0;
    unsigned int nodes = 0;
    unsigned int movetime = 0;
    unsigned int wtime = 0;
    unsigned int btime = 0;
    unsigned int winc = 0;
    unsigned int binc = 0;
    unsigned int movestogo = 0;
    bool infinite = false;
};

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
        quiesceSearch();
        board->setFen(fen, &states[0]);
    }

    void go(const GoParams& params);
    void goPerft(unsigned int depth);

    /**
     * @brief Resizes the transposition table. @param megabytes new size in MB.
     */
    void SetHashSize(unsigned int megabytes);

    /**
     * @brief Fixed-depth search over a fixed position set; prints "<N> nodes <M> nps".
     *
     * The node total is deterministic for a given binary, so it doubles as a regression check:
     * a refactor that is meant to be non-functional must not change it.
     */
    void bench(unsigned int depth);

    /**
     * @brief Runs perft over the standard positions and checks against known node counts.
     * @return true if every position matched.
     */
    bool perftSuite();

    void stop();

    void eval();

    void makemove(Move move);
    void undomove()
    {
        quiesceSearch();
        board->undoMove();
    }

    void isCheck(Move move);
    void ClearTT()
    {
        searcher->ClearTT();
    }

  private:
    /**
     * @brief Stops any in-flight search and waits for the worker to actually finish.
     *
     * The searcher gets a *shallow* copy of the Board, so its `state` pointer aliases this
     * object's `states[]` array. Mutating the board while the worker is still unwinding is a
     * genuine data race -- and `stop` only sets a flag, so the UCI thread can reach a following
     * `position` command well before the worker has let go.
     */
    void quiesceSearch()
    {
        searcher->Stop();
        searcher->WaitForSearch();
    }

    Board* board;
    Searcher* searcher;
    BoardState states[MAX_PLY + 1]; // plus 1 for starting point
};

#endif