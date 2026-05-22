#ifndef LAYER_H
#define LAYER_H

#include <cstdint>
#include <cstring>

#include "halfkav2_hm.h"

struct Accumulator
{
    int32_t data[520];
};

template <typename W_T, typename B_T, size_t IN, size_t OUT, int SCALE = 64, int32_t MAX = 127>
struct Layer
{
    using weight_t = W_T;
    using bias_t = B_T;

    static constexpr size_t in_size = IN;
    static constexpr size_t out_size = OUT;
    static constexpr int scale = SCALE;
    static constexpr int half_scale = SCALE / 2;
    static constexpr int32_t max = MAX;

    // Layer parameters
    weight_t weights[out_size][in_size];
    bias_t biases[out_size];

    constexpr int32_t CReLU(int32_t x, int32_t max) const
    {
        return std::min(std::max(x, 0), max);
    }

    inline void Forward(const Accumulator& us, const Accumulator& them, int32_t* output) const
    {
        for (size_t i = 0; i < out_size; i++)
        {
            int32_t sum = biases[i];
            for (size_t j = 0; j < in_size / 2; j++)
            {
                sum += weights[i][j] * CReLU(us.data[j], max);
                sum += weights[i][j + in_size / 2] * CReLU(them.data[j], max);
            }
            output[i] = (sum + half_scale) / scale; // Round to nearest integer
        }
    }

    inline void Forward(const int32_t* input, int32_t* output) const
    {
        for (size_t i = 0; i < out_size; i++)
        {
            int32_t sum = biases[i];
            for (size_t j = 0; j < in_size; j++)
            {
                sum += static_cast<int32_t>(weights[i][j]) * CReLU(input[j], max);
            }
            output[i] = (sum + half_scale) / scale; // Round to nearest integer
        }
    }

    inline void Forwardf(const int32_t* input, float* output) const
    {
        for (size_t i = 0; i < out_size; i++)
        {
            int32_t sum = biases[i];
            for (size_t j = 0; j < in_size; j++)
            {
                sum += static_cast<int32_t>(weights[i][j]) * CReLU(input[j], max);
            }
            output[i] = static_cast<float>(sum) / static_cast<float>(scale);
        }
    }
};

template <size_t IN, size_t OUT>
using HiddenLayer = Layer<int8_t, int32_t, IN, OUT>;

using InputLayer = Layer<int16_t, int16_t, NUM_FEATURES, 520>;

#endif // LAYER_H