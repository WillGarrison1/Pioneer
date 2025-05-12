#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "types.h"


extern Move startSearch(Board& board, unsigned int depth);

#endif