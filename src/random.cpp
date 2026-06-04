#include "random.h"
#include <random>
#include <time.h>

unsigned long long RandNum()
{
    static std::mt19937_64 rng(0);//(unsigned long long)time(nullptr));
    return rng();
}