#ifndef LAYER_H
#define LAYER_H

#include <cstdint>
#include <cstring>

#include "accumulator.h"
#include "halfkav2_hm.h"
#include "../SIMD.h"

template <typename W_T, typename B_T, size_t IN, size_t OUT, int SCALE = 64, int32_t MAX = 127, bool IS_INPUT = false>
struct Layer
{
    using weight_t = W_T;
    using bias_t = B_T;

    static constexpr bool isInputLayer = IS_INPUT;
    static constexpr size_t in_size = IN;
    static constexpr size_t out_size = OUT;
    static constexpr size_t row = isInputLayer ? in_size : out_size;
    static constexpr size_t column = isInputLayer ? out_size : in_size;
    static constexpr int scale = SCALE;
    static constexpr int half_scale = SCALE / 2;
    static constexpr int32_t max = MAX;

    // Layer parameters
    alignas(64) weight_t weights[row][column];
    alignas(64) bias_t biases[out_size];

    constexpr int32_t CReLU(const int32_t x, const int32_t max) const
    {
        return std::min(std::max(x, 0), max);
    }

    inline void Forward(const Accumulator& us, const Accumulator& them, int32_t* output) const
    {
        static_assert(!isInputLayer, "Cannot call forward on input layer!");

        alignas(32) int8_t us_relu[in_size / 2];
        alignas(32) int8_t them_relu[in_size / 2];

        SIMD::CReLU(us.data, us_relu, max, in_size / 2);
        SIMD::CReLU(them.data, them_relu, max, in_size / 2);

        for (uint32_t i = 0; i < out_size; i++)
        {
            const int8_t* __restrict w = weights[i];

            int32_t sum = biases[i];
            
            sum += SIMD::dotProduct8(us_relu, w, in_size / 2);
            sum += SIMD::dotProduct8(them_relu, w + in_size / 2, in_size / 2);

            output[i] = (sum + half_scale) / scale; // Round to nearest integer
        }
    }

    inline void Forward(const int32_t* input, int32_t* output) const
    {
        static_assert(!isInputLayer, "Cannot call forward on input layer!");

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
        static_assert(!isInputLayer, "Cannot call forward on input layer!");

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

using InputLayer = Layer<int16_t, int16_t, NUM_FEATURES, 520, 64, 127, true>;

#endif // LAYER_H