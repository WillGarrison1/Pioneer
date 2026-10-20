#include "accumulatorList.h"

#include "../board.h"
#include "../piece.h"
#include "accumulator.h"
#include "nnue.h"

AccumulatorList::AccumulatorList() : last(0)
{
    accumulators = new (std::align_val_t(64)) AccumulatorNode[MAX_DEPTH]{};
}

AccumulatorList::~AccumulatorList()
{
    operator delete[](accumulators, std::align_val_t(64));
}

template <Color POV>
void AccumulatorList::ApplyDirtyMove(Accumulator &toAcc, const Accumulator &fromAcc, const DirtyMove &dirtyMove, Square king_sq)
{
    constexpr bool whitePOV = POV == WHITE;
    if (dirtyMove.movePiece == EMPTY) // null-move
    {
        toAcc = fromAcc;
        return;
    }

    Piece fromPiece, toPiece;
    fromPiece = dirtyMove.movePiece;

    if (dirtyMove.promote == EMPTY)
    {
        toPiece = fromPiece;
    }
    else
    {
        toPiece = dirtyMove.promote;
    }

    int fromIndex = GetIndex(dirtyMove.from, king_sq, fromPiece, whitePOV);
    int toIndex = GetIndex(dirtyMove.to, king_sq, toPiece, whitePOV);

    if (dirtyMove.castleFrom != SQ_NONE)
    {
        Piece rook = makePiece(ROOK, getColor(fromPiece));
        int castleFrom = GetIndex(dirtyMove.castleFrom, king_sq, rook, whitePOV);
        int castleTo = GetIndex(dirtyMove.castleTo, king_sq, rook, whitePOV);
        nnue->AddAddSubSub(toAcc, fromAcc, toIndex, castleTo, fromIndex, castleFrom);
    }
    else if (dirtyMove.capturedPiece != EMPTY)
    {
        int captureIndex = GetIndex(dirtyMove.captured, king_sq, dirtyMove.capturedPiece, whitePOV);
        nnue->AddSubSub(toAcc, fromAcc, toIndex, fromIndex, captureIndex);
    }
    else
    {
        nnue->AddSub(toAcc, fromAcc, toIndex, fromIndex);
    }
}

template <Color POV>
void AccumulatorList::UpdateAccumulator(const Board &board)
{
    constexpr bool whitePOV = POV == WHITE;
    constexpr Piece ourKing = makePiece(KING, POV);

    AccumulatorNode* lastNode = &accumulators[last];

    if (lastNode->IsComputed<POV>())
        return;

    

    
    int start = last;
    bool refresh = false;
    while (start >= 0 && !refresh && !accumulators[start].IsComputed<POV>())
        refresh = accumulators[start--].dirtyMove.movePiece == ourKing;
    
    if (refresh)
    {
        if constexpr (whitePOV)
        {    
            board.ResetWhiteAccumulator(accumulators[last].whiteAcc);
            accumulators[last].isWhiteComputed = true;
        }
        else
        {    
            board.ResetBlackAccumulator(accumulators[last].blackAcc);
            accumulators[last].isBlackComputed = true;
        }
        return;
    }

    assert(start>=0);
    const Square kingSqr = lsb(board.getBB(POV, KING));
    for (int i=start+1; i<=last; i++)
    {
        const Accumulator& prev = whitePOV ? accumulators[i - 1].whiteAcc : accumulators[i - 1].blackAcc;
        Accumulator& cur = whitePOV ? accumulators[i].whiteAcc : accumulators[i].blackAcc;
        ApplyDirtyMove<POV>(cur, prev, accumulators[i].dirtyMove, kingSqr);
        (whitePOV ? accumulators[i].isWhiteComputed : accumulators[i].isBlackComputed) = true;
    }
}

void AccumulatorList::ComputeAccumulators(const Board& board)
{
    UpdateAccumulator<WHITE>(board);
    UpdateAccumulator<BLACK>(board);

    #ifdef VERIFY_ACCUMULATOR
    Accumulator refWhite, refBlack;
    board.ResetWhiteAccumulator(refWhite);
    board.ResetBlackAccumulator(refBlack);
    assert(std::memcmp(refWhite.data, accumulators[last].whiteAcc.data, sizeof(refWhite.data)) == 0);
    assert(std::memcmp(refBlack.data, accumulators[last].blackAcc.data, sizeof(refBlack.data)) == 0);
    assert(std::memcmp(refWhite.psqt, accumulators[last].whiteAcc.psqt, sizeof(refWhite.psqt)) == 0);
    assert(std::memcmp(refBlack.psqt, accumulators[last].blackAcc.psqt, sizeof(refBlack.psqt)) == 0);
    #endif

}

