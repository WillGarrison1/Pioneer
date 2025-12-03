#include "random.h"
#include <random>
#include <time.h>

unsigned long long RandNum()
{
    static std::mt19937_64 rng((unsigned long long)time(nullptr));
    return rng();
}