#ifndef NNUE_H
#define NNUE_H

#include "../types.h"
#include "layer.h"
#include "subnet.h"
#include <string>
#include <vector>

#define NUM_SUBNETS 8

class NNUE
{
  public:
    NNUE() = default;
    ~NNUE() = default;

    /**
     * Loads the NNUE parameters from a file. The file should be in a specific format, with the input layer parameters followed by the subnet parameters.
     * Returns true if the parameters were successfully loaded, false otherwise.
     */
    bool Load(const std::string& filename);

    /**
     * Evaluates the given board position using the NNUE
     */
    float Evaluate(const Board& board) const;

    /**
     * Evaluates the given board position using the NNUE using only the psqt values (much faster but much less positional information)
     */
    float FastEvaluate(const Board& board) const;

    /**
     * Adds the contribution of the given index to the accumulator.
     */
    void Add(Accumulator& acc, int index) const;

    /**
     * Removes the contribution of the given index from the accumulator.
     */
    void Remove(Accumulator& acc, int index) const;
    
    /**
     * Updates the accumulator by removing the contribution of the old index and adding the contribution of the new index.
     */
    void Update(Accumulator& acc, int oldIndex, int newIndex) const;

    /**
     * Resets the accumulator to the initial state (only biases)
     */
    void Reset(Accumulator& acc) const;

  private:
    /**
     * Feeds the accumulators through the subnets and returns the final evaluation. The "us" accumulator should contain the contributions of our pieces, while the "them" accumulator should contain the contributions of the opponent's pieces.
     */
    float Forward(int subnet, const Accumulator& us, const Accumulator& them) const;

    InputLayer inputLayer;
    Subnet subnets[NUM_SUBNETS];
};

extern NNUE* nnue;

#endif // NNUE_H