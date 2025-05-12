#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "move.h"
#include "types.h"


inline Bitboard slidingMoves(Square s, PieceType t, Bitboard blockers) {
  constexpr Direction slideDirs[] = {
      NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST};

  const int start = t == QUEEN || t == ROOK ? 0 : 4;
  const int end = t == QUEEN || t == BISHOP ? 8 : 4;

  Bitboard moves = 0ULL;

  for (int i = start; i < end; i++) {
    moves |= sendRay(s, slideDirs[i], blockers);
  }

  return moves;
}
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