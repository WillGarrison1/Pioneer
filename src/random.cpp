#include "random.h"
#include <random>
#include <time.h>

unsigned long long RandNum()
{
    static std::mt19937_64 rng(123ULL); //(unsigned long long)time(nullptr));
    return rng();
}