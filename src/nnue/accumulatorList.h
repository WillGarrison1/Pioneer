#ifndef ACCUMULATOR_LIST_H
#define ACCUMULATOR_LIST_H

#include "../types.h"
#include "accumulator.h"

#include <cassert>

class AccumulatorList
{
  public:
    AccumulatorList();
    ~AccumulatorList();

    void ComputeAccumulator(const Board& board);

    inline AccumulatorNode& Current()
    {
        return accumulators[last];
    }

    // The array holds exactly MAX_DEPTH nodes. Search bounds ply via PLY_LIMIT; this asserts that
    // invariant actually holds, since violating it is a multi-KB out-of-bounds heap write rather
    // than anything that would show up as a wrong result.
    inline void SetCurrent(int cur)
    {
        assert(cur >= 0 && cur < MAX_DEPTH);
        last = cur;
    }

  private:
    AccumulatorNode* accumulators;
    int last;
};

#endif