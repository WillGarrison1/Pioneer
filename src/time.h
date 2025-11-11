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
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

inline unsigned long long getTimeNS()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#endif