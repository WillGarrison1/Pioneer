#ifndef ACCUMULATOR_LIST_H
#define ACCUMULATOR_LIST_H

#include "../types.h"
#include "accumulator.h"

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

    inline void SetCurrent(int cur)
    {
        last = cur;
    }

  private:
    AccumulatorNode* accumulators;
    int last;
};

#endif