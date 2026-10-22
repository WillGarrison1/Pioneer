#include "option.h"
#include <iostream>

void SpinOption::Set(const std::string& input)
{
    int val = std::stoi(input);
    if (val < min || val > max)
    {
        std::cout << "info string inputted value `" << val << "` isn't between option range " << min << "-" << max
                  << std::endl;
        return;
    }
    cur = val;
    on_update(cur);

    std::cout << "info string " << name << " set to " << cur << std::endl;
}

void CheckOption::Set(const std::string& input)
{
    if (input == "false")
    {
        cur = false;
    }
    else if (input == "true")
    {
        cur = true;
    }
    else
    {
        std::cout << "info string unknown input for check type " << input << std::endl;
        return;
    }

    on_update(cur);
    std::cout << "info string " << name << " set to " << input << std::endl;
}

void StringOption::Set(const std::string& input)
{
    cur = input;
    on_update(cur);
    std::cout << "info string " << name << " set to " << input << std::endl;
}