#include "random.h"
#include <random>

#define RANDOM_SEED 0

unsigned long long RandNum()
{
#ifdef RANDOM_SEED
    static std::mt19937_64 rng(RANDOM_SEED);
#else
    static std::mt19937_64 rng((unsigned long long)time(nullptr));
#endif
    return rng();
}