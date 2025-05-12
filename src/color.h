#ifndef COLOR_H
#define COLOR_H

#include "types.h"

constexpr Color operator~(Color color)
{
    return static_cast<Color>(color ^ 8);
}

#endif
