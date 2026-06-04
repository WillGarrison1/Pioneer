#ifndef SIMD_H
#define SIMD_H

#include <immintrin.h>
#include <algorithm>


namespace SIMD
{
    inline int32_t dotProduct8(const int8_t* a, const int8_t* b, int32_t* result, int n);
    inline void CReLU(const int16_t* input, int8_t* output, int32_t max, int n);


    #ifdef __AVX512BW__
    
        inline int32_t dotProduct8(const int8_t* a, const int8_t* b, int n)
        {
            __m512i sum = _mm512_setzero_si512();
            __m512i ones = _mm512_set1_epi16(1);
            for (int i = 0; i < n; i += 64)
            {
                __m512i vecA = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(a + i));
                __m512i vecB = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(b + i));
                __m512i product = _mm512_maddubs_epi16(vecA, vecB);
                sum = _mm512_add_epi32(sum, _mm512_madd_epi16(product, ones));
            }
            return _mm512_reduce_add_epi32(sum);
        }

        inline void CReLU(const int16_t* input, int8_t* output, int32_t max, int n)
        {
            __m512i maxVec = _mm512_set1_epi16(max);
            __m512i zeroVec = _mm512_setzero_si512();
            for (int i = 0; i < n; i += 64)
            {
                __m512i veca = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i));
                __m512i vecb = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(input + i + 32));
                veca = _mm512_min_epi16(_mm512_max_epi16(veca, zeroVec), maxVec);
                vecb = _mm512_min_epi16(_mm512_max_epi16(vecb, zeroVec), maxVec);
                __m512i packed = _mm512_packs_epi16(veca, vecb);
                const __m512i idx = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);
                packed = _mm512_permutexvar_epi64(idx, packed);
                _mm512_storeu_si512(reinterpret_cast<__m512i*>(output + i), packed);
            }
        }
    #else
    #ifdef __AVX2__

        inline int32_t dotProduct8(const int8_t* a, const int8_t* b, int n)
        {
            __m256i sum = _mm256_setzero_si256();
            __m256i ones = _mm256_set1_epi16(1);
            for (int i = 0; i < n; i += 32)
            {
                __m256i vecA = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
                __m256i vecB = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
                __m256i product = _mm256_maddubs_epi16(vecA, vecB);
                sum = _mm256_add_epi32(sum, _mm256_madd_epi16(product, ones));
            }
            
            __m128i low = _mm256_extracti128_si256(sum, 0);
            __m128i high = _mm256_extracti128_si256(sum, 1);
            __m128i s = _mm_add_epi32(low, high);
            s = _mm_add_epi32(s, _mm_unpackhi_epi64(s, s));
            s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
            return _mm_cvtsi128_si32(s);
        }

        inline void CReLU(const int16_t* input, int8_t* output, int32_t max, int n)
        {
            __m256i maxVec = _mm256_set1_epi16(max);
            __m256i zeroVec = _mm256_setzero_si256();
            for (int i = 0; i < n; i += 32)
            {
                __m256i veca = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));
                __m256i vecb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i + 16));
                veca = _mm256_min_epi16(_mm256_max_epi16(veca, zeroVec), maxVec);
                vecb = _mm256_min_epi16(_mm256_max_epi16(vecb, zeroVec), maxVec);
                __m256i result = _mm256_packs_epi16(veca, vecb);
                result = _mm256_permute4x64_epi64(result, 0xD8); // Shuffle to correct order
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), result);
            }
        }

    #else
        inline void CReLU(const int16_t* input, int8_t* output, int32_t max, int n)
        {
            for (int i = 0; i < n; i++)
            {
                output[i] = std::min(std::max(input[i], static_cast<int16_t>(0)), static_cast<int16_t>(max));
            }
        }

        inline int32_t dotProduct8(const int8_t* a, const int8_t* b, int n)
        {
            int result = 0;
            for (int i = 0; i < n; i++)
            {
                result += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
            }
            return result;
        }
    #endif
    #endif
}

#endif
