#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "move.h"
#include "types.h"
#include "magic.h"

template <MoveType type, Color color>
extern void generateMoves(Board &board, MoveList *list);

#endif // MOVEGEN_H