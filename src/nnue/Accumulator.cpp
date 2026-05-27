#include "accumulator.h"
#include "nnue.h"

void Accumulator::Add(Square square, Square kingSquare, Piece piece, bool whitePOV)
{
    int index = GetIndex(square, kingSquare, piece, whitePOV);
    nnue->Add(*this, index);
}

void Accumulator::Remove(Square square, Square kingSquare, Piece piece, bool whitePOV)
{
    int index = GetIndex(square, kingSquare, piece, whitePOV);
    nnue->Remove(*this, index);
}

void Accumulator::Update(Square from, Square to, Square kingSquare, Piece fromPiece, Piece toPiece, bool whitePOV)
{
    int fromIndex = GetIndex(from, kingSquare, fromPiece, whitePOV);
    int toIndex = GetIndex(to, kingSquare, toPiece, whitePOV);
    nnue->Update(*this, fromIndex, toIndex);
}
