#ifndef SIMD_H
#define SIMD_H

#include <immintrin.h>
#include <algorithm>

namespace SIMD
{


    #ifdef __AVX512BW__

        using vec_t = __m512i;

        inline vec_t loadVec(const int8_t* ptr)
        {
            return _mm512_loadu_si512(reinterpret_cast<const vec_t*>(ptr));
        }

        inline void storeVec(int8_t* ptr, const vec_t& v)
        {
            _mm512_storeu_si512(reinterpret_cast<vec_t*>(ptr), v);
        }

        inline vec_t pack(const vec_t& v, const vec_t& w)
        {
            vec_t pack = _mm512_packs_epi16(v, w);
            const __m512i idx = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);
            return _mm512_permutexvar_epi64(idx, pack);
        }

        inline vec_t vecMaddubs16(const vec_t& a, const vec_t& b)
        {
            return _mm512_maddubs_epi16(a, b);
        }

        inline vec_t vecZero()
        {
            return _mm512_setzero_si512();
        }

        inline vec_t vecOnes()
        {
            return _mm512_set1_epi16(1);
        }

        inline vec_t vecBroadcast8(int8_t num)
        {
            return _mm512_set1_epi8(num);
        }

        inline vec_t vecBroadcast16(int16_t num)
        {
            return _mm512_set1_epi16(num);
        }

        inline vec_t vecAddAdjacents16(const vec_t& v)
        {
            return _mm512_madd_epi16(v, vecOnes());
        }

        inline vec_t vecAdd32(const vec_t& a, const vec_t& b)
        {
            return _mm512_add_epi32(a, b);
        }

        inline vec_t vecMul32(const vec_t& a, const vec_t& b)
        {
            return _mm512_mullo_epi32(a, b);
        }

        inline vec_t vecAdd16(const vec_t& a, const vec_t& b)
        {
            return _mm512_add_epi16(a, b);
        }

        inline vec_t vecMul16(const vec_t& a, const vec_t& b)
        {
            return _mm512_mullo_epi16(a, b);
        }

        inline vec_t shiftRight16(const vec_t& v, int count)
        {
            return _mm512_srai_epi16(v, count);
        }

        inline vec_t vecAdd8(const vec_t& a, const vec_t& b)
        {
            return _mm512_add_epi8(a, b);
        }

        inline vec_t vecMin16(const vec_t& a, const vec_t& b)
        {
            return _mm512_min_epi16(a, b);
        }

        inline vec_t vecMax16(const vec_t& a, const vec_t& b)
        {
            return _mm512_max_epi16(a, b);
        }

        inline int32_t vecReduceAdd(const vec_t& v)
        {
            return _mm512_reduce_add_epi32(v);
        }
    #else
    #ifdef __AVX2__

        using vec_t = __m256i;

        inline vec_t loadVec(const int8_t* ptr)
        {
            return _mm256_loadu_si256(reinterpret_cast<const vec_t*>(ptr));
        }

        inline void storeVec(int8_t* ptr, const vec_t& v)
        {
            _mm256_storeu_si256(reinterpret_cast<vec_t*>(ptr), v);
        }

        inline vec_t pack(const vec_t& v, const vec_t& w)
        {
            vec_t pack = _mm256_packs_epi16(v, w);
            return _mm256_permute4x64_epi64(pack, 0xD8); // Shuffle to correct order
        }

        inline vec_t vecMaddubs16(const vec_t& a, const vec_t& b)
        {
            return _mm256_maddubs_epi16(a, b);
        }

        inline vec_t vecZero()
        {
            return _mm256_setzero_si256();
        }

        inline vec_t vecOnes()
        {
            return _mm256_set1_epi16(1);
        }

        inline vec_t vecBroadcast8(const int8_t num)
        {
            return _mm256_set1_epi8(num);
        }

        inline vec_t vecBroadcast16(const int16_t num)
        {
            return _mm256_set1_epi16(num);
        }

        inline vec_t vecAddAdjacents16(const vec_t& v)
        {
            return _mm256_madd_epi16(v, vecOnes());
        }

        inline vec_t vecAdd32(const vec_t& a, const vec_t& b)
        {
            return _mm256_add_epi32(a, b);
        }

        inline vec_t vecMul32(const vec_t& a, const vec_t& b)
        {
            return _mm256_mullo_epi32(a, b);
        }

        inline vec_t vecAdd16(const vec_t& a, const vec_t& b)
        {
            return _mm256_add_epi16(a, b);
        }

        inline vec_t vecMul16(const vec_t& a, const vec_t& b)
        {
            return _mm256_mullo_epi16(a, b);
        }

        inline vec_t vecAdd8(const vec_t& a, const vec_t& b)
        {
            return _mm256_add_epi8(a, b);
        }

        inline vec_t shiftRight16(const vec_t& v, int count)
        {
            return _mm256_srai_epi16(v, count);
        }

        inline vec_t vecMin16(const vec_t& a, const vec_t& b)
        {
            return _mm256_min_epi16(a, b);
        }

        inline vec_t vecMax16(const vec_t& a, const vec_t& b)
        {
            return _mm256_max_epi16(a, b);
        }

        inline int32_t vecReduceAdd(const vec_t& v)
        {
            __m128i low = _mm256_extracti128_si256(v, 0);
            __m128i high = _mm256_extracti128_si256(v, 1);
            __m128i s = _mm_add_epi32(low, high);
            s = _mm_add_epi32(s, _mm_unpackhi_epi64(s, s));
            s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
            return _mm_cvtsi128_si32(s);
        }

    #else
        using vec_t = __m128i;

        inline vec_t loadVec(const int8_t* ptr)
        {
            return _mm_loadu_si128(reinterpret_cast<const vec_t*>(ptr));
        }

        inline void storeVec(int8_t* ptr, const vec_t& v)
        {
            _mm_storeu_si128(reinterpret_cast<vec_t*>(ptr), v);
        }

        inline vec_t pack(const vec_t& v, const vec_t& w)
        {
            return _mm_packs_epi16(v, w);
        }

        inline vec_t vecMaddubs16(const vec_t& a, const vec_t& b)
        {
            return _mm_maddubs_epi16(a, b);
        }

        inline vec_t vecZero()
        {
            return _mm_setzero_si128();
        }

        inline vec_t vecOnes16()
        {
            return _mm_set1_epi16(1);
        }

        inline vec_t vecBroadcast8(const int8_t num)
        {
            return _mm_set1_epi8(num);
        }

        inline vec_t vecBroadcast16(const int16_t num)
        {
            return _mm_set1_epi16(num);
        }

        inline vec_t vecAddAdjacents16(const vec_t& v)
        {
            return _mm_madd_epi16(v, vecOnes16());
        }

        inline vec_t vecAdd32(const vec_t& a, const vec_t& b)
        {
            return _mm_add_epi32(a, b);
        }

        inline vec_t vecMul32(const vec_t& a, const vec_t& b)
        {
            return _mm_mullo_epi32(a, b);
        }

        inline vec_t vecAdd16(const vec_t& a, const vec_t& b)
        {
            return _mm_add_epi16(a, b);
        }

        inline vec_t vecMul16(const vec_t& a, const vec_t& b)
        {
            return _mm_mullo_epi16(a, b);
        }

        inline vec_t shiftRight16(const vec_t& v, int count)
        {
            return _mm_srai_epi16(v, count);
        }

        inline vec_t vecAdd8(const vec_t& a, const vec_t& b)
        {
            return _mm_add_epi8(a, b);
        }
        inline vec_t vecMin16(const vec_t& a, const vec_t& b)
        {
            return _mm_min_epi16(a, b);
        }

        inline vec_t vecMax16(const vec_t& a, const vec_t& b)
        {
            return _mm_max_epi16(a, b);
        }

        inline int32_t vecReduceAdd(const vec_t& v)
        {
            __m128i s = _mm_add_epi32(v, _mm_unpackhi_epi64(v, v));
            s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
            return _mm_cvtsi128_si32(s);
        }
    #endif
    #endif

    inline int32_t dotProduct8(const int8_t* a, const int8_t* b, int n)
    {
        vec_t zero = vecZero();
        vec_t sum = zero;
        for (int i = 0; i < n; i += sizeof(vec_t))
        {
            vec_t vecA = loadVec(a + i);
            vec_t vecB = loadVec(b + i);
            vec_t product = vecMaddubs16(vecA, vecB);
            sum = vecAdd32(sum, vecAddAdjacents16(product));
        }
        return vecReduceAdd(sum);
    }

    inline void CReLU(const int16_t* input, int8_t* output, int32_t max, int n)
    {
        vec_t maxVec = vecBroadcast16(max);
        vec_t zeroVec = vecZero();

        constexpr int W = sizeof(vec_t) / sizeof(input[0]);

        for (int i = 0; i < n; i += W * 2)
        {
            vec_t veca = loadVec(reinterpret_cast<const int8_t*>(input + i));
            vec_t vecb = loadVec(reinterpret_cast<const int8_t*>(input + i + W));
            veca = vecMin16(vecMax16(veca, zeroVec), maxVec);
            vecb = vecMin16(vecMax16(vecb, zeroVec), maxVec);
            vec_t packed = pack(veca, vecb); // Pack to 8-bit integers
            storeVec(output + i, packed);
        }
    }

    inline void SqrCReLU(const int16_t* input, int8_t* output, int32_t max, int n)
    {
        vec_t maxVec = vecBroadcast16(max);
        vec_t zeroVec = vecZero();

        int half = n / 2;
        constexpr int W = sizeof(vec_t) / sizeof(input[0]);

        for (int i = 0; i < half; i += W * 2)
        {
            vec_t veca1 = loadVec(reinterpret_cast<const int8_t*>(input + i));
            vec_t veca2 = loadVec(reinterpret_cast<const int8_t*>(input + half + i));
            vec_t vecb1 = loadVec(reinterpret_cast<const int8_t*>(input + i + W));
            vec_t vecb2 = loadVec(reinterpret_cast<const int8_t*>(input + half + i + W));
            veca1 = vecMin16(vecMax16(veca1, zeroVec), maxVec);
            veca2 = vecMin16(vecMax16(veca2, zeroVec), maxVec);
            vecb1 = vecMin16(vecMax16(vecb1, zeroVec), maxVec);
            vecb2 = vecMin16(vecMax16(vecb2, zeroVec), maxVec);
            vec_t veca = vecMul16(veca1, veca2);
            vec_t vecb = vecMul16(vecb1, vecb2);
            veca = shiftRight16(veca, 7);
            vecb = shiftRight16(vecb, 7);
            vec_t packed = pack(veca, vecb); // Pack to 8-bit integers
            storeVec(output + i, packed);
        }
    }
}

#endif
