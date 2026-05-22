#ifndef PLATFORM_H
#define PLATFORM_H
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
inline void GetExecutablePath(std::string& path)
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    path = std::string(buffer);
}
#elif defined(__linux__)
#include <linux/limits.h>
#include <unistd.h>
inline void GetExecutablePath(std::string& path)
{
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        path = std::string(buffer);
    }
    else
    {
        path = "";
    }
}
#else
#error "Unsupported platform"
#endif

#endif