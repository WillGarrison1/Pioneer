#ifndef TIME_H
#define TIME_H

#include <chrono>
#include <iostream>

/**
 * @brief Get the time in milliseconds
 *
 * @return unsigned long long
 */
inline unsigned long long getTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

#define BENCHMARK() Benchmark __bench__

class Benchmark
{
  public:
    Benchmark() : start(getTime())
    {
    }
    ~Benchmark()
    {
        std::cout << "Took: " << getTime() - start << " ms" << std::endl;
    }

  private:
    unsigned long long start;
};

#endif