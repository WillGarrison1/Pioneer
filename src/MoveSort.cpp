#include "MoveSort.h"

Move killerMoves[MAX_PLY][2]; // each ply can have two killer moves
int moveHistory[2][64][64];   // History for [isBlack][from][to]