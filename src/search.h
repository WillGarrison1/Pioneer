#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "types.h"

enum NodeType
{
    PVNode,
    CUTNode,
};

/**
 * @brief Searches for the best chess move
 *
 * @param board the current position
 * @param depth how many moves deep to search (if zero then max moves)
 * @param nodes how many nodes to search (if zero then infinite)
 * @param movetime how many milliseconds to take when searching (if zero then infinite)
 * @param movetime how much time is remaining for the bot in a game (if zero then default to movetime)
 * @return The best move
 */
extern Move startSearch(Board &board, unsigned int depth, unsigned int nodes, unsigned int movetime, unsigned int remaining_time);

#endif