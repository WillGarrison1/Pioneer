#ifndef PROF_H
#define PROF_H

#define NO_PROFILE

#include <string>
#include <vector>

class ProfileManager;

struct FunctionEntry
{
    unsigned long long count; // number of times the function is called
    unsigned long long time;  // total time spent on function
    std::string funcName;
    ProfileManager *manager;

    FunctionEntry(ProfileManager *manager, std::string name);
    ~FunctionEntry();
};

class ProfileManager
{
public:
    ProfileManager() {}
    ~ProfileManager();

    inline void AddEntry(FunctionEntry *entry)
    {
        functionEntries.emplace_back(entry);
    }

private:
    struct FunctionEntryInternal
    {
        FunctionEntryInternal(FunctionEntry *entry)
        {
            count = entry->count;
            time = entry->time;
            funcName = entry->funcName;
        }

        unsigned long long count; // number of times the function is called
        unsigned long long time;  // total time spent on function
        std::string funcName;
    };
    std::vector<FunctionEntryInternal> functionEntries;
};

class Profiler
{

public:
    Profiler(FunctionEntry *entry);
    ~Profiler();

private:
    FunctionEntry *entry;
    unsigned long long startTime;
};

extern ProfileManager *GetManager();

#ifndef NO_PROFILE

#define PROFILE_FUNC()                                   \
    static FunctionEntry _entry(GetManager(), __func__); \
    Profiler _prof(&_entry);

#else

#define PROFILE_FUNC()

#endif

#endif