#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "move.h"
#include "types.h"
#include "magic.h"

template <MoveType type>
extern void generateMoves(Board &board, MoveList *list);

template <PieceType pieceType>
extern void generatePieceMoves(const Board &board, MoveList *list);

#endif // MOVEGEN_H