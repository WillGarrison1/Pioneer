#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "move.h"
#include "types.h"
#include "magic.h"

template <MoveType mType>
extern void generatePawnMoves(const Board &board, MoveList *list);
template <MoveType mType>
extern void generateKnightMoves(const Board &board, MoveList *list);
template <MoveType mType>
extern void generateBishopMoves(const Board &board, MoveList *list);
template <MoveType mType>
extern void generateRookMoves(const Board &board, MoveList *list);
template <MoveType mType>
extern void generateQueenMoves(const Board &board, MoveList *list);
template <MoveType mType>
extern void generateKingMoves(const Board &board, MoveList *list);

template <MoveType type>
extern void generateMoves(Board &board, MoveList *list);

#endif // MOVEGEN_H