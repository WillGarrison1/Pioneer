#ifndef UCI_H
#define UCI_H

#include "engine.h"

class Interface
{
public:
    Interface();
    ~Interface();

    void run();

private:
    Engine engine;
};

#endif