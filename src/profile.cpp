#include "profile.h"
#include "time.h"

#include <fstream>

ProfileManager *GetManager()
{
    static ProfileManager manager;
    return &manager;
}

FunctionEntry::FunctionEntry(ProfileManager *manager, std::string name) : funcName(name), count(0), time(0), manager(manager)
{
}

FunctionEntry::~FunctionEntry()
{
    manager->AddEntry(this);
}

Profiler::Profiler(FunctionEntry *entry) : entry(entry)
{
    entry->count++;
    this->startTime = getTimeNS();
}

Profiler::~Profiler()
{
    this->entry->time += getTimeNS() - this->startTime;
}

ProfileManager::~ProfileManager()
{
    std::fstream profileF("pioneer.prof", std::ios_base::out | std::ios_base::trunc);
    profileF << "Name | Count | Time | Avg-Runtime\n";
    for (FunctionEntryInternal entry : this->functionEntries)
    {
        profileF << entry.funcName << ", " << entry.count << ", " << (double)entry.time / 1e9 << "s, " << (double)entry.time / (double)entry.count << "ns\n";
    }
    profileF.close();
}