#include "accumulatorList.h"

#include "../board.h"
#include "../piece.h"
#include "nnue.h"

AccumulatorList::AccumulatorList() : last(0)
{
    accumulators = new (std::align_val_t(64)) AccumulatorNode[MAX_DEPTH]{};
}

AccumulatorList::~AccumulatorList()
{
    operator delete(accumulators, std::align_val_t(64));
}

void AccumulatorList::ComputeAccumulator(const Board& board)
{
    Square whiteKingSquare = lsb(board.getBB(WHITE, KING));
    Square blackKingSquare = lsb(board.getBB(BLACK, KING));

    int lastComputedBlack = last;
    int lastComputedWhite = last;
    bool needsFullRefreshBlack = false;

    // check if full refresh is needed
    while (lastComputedBlack >= 0 && !accumulators[lastComputedBlack].isBlackComputed && !needsFullRefreshBlack)
    {
        needsFullRefreshBlack = accumulators[lastComputedBlack--].dirtyMove.movePiece == makePiece(KING, BLACK);
    }

    assert(lastComputedBlack >= 0);

    auto& accumulatorNode = accumulators[last];
    auto& blackComputed = accumulators[lastComputedBlack];

    if (needsFullRefreshBlack)
    {
        board.ResetBlackAccumulator(accumulatorNode.blackAcc);
    }
    else
    {
        /*
            Possible optimization for "smart-updating" the nnue. It minimizes adds and removes.
            Smartly updating the accumulator happens in two stages:
            1. Add new dirty pieces to the accumulator
            2. Remove old dirty pieces from the accumulator

            The idea for computing follows these rules:
            1. When coming across an untracked dirty piece (new dirty piece), we add it to the current accumulator.
            Captured pieces will be treated as dirty pieces, but aren't added them to the accumulator
            3. When coming across a tracked dirty piece, update it's position on the piece table so we can find it's
           origin at the last computed accumulator
            4. When reaching the previous computed accumulator, remove all the tracked pieces from the current
           accumulator
        */
        Accumulator& acc = accumulatorNode.blackAcc;
        acc = blackComputed.blackAcc;

        int current = last;

        while (current != lastComputedBlack)
        {

            const auto& dirtyMove = accumulators[current].dirtyMove;
            if (dirtyMove.movePiece == EMPTY) // null-move
            {
                current--;
                continue;
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

            int fromIndex = GetIndex(dirtyMove.from, blackKingSquare, fromPiece, false);
            int toIndex = GetIndex(dirtyMove.to, blackKingSquare, toPiece, false);

            if (dirtyMove.castleFrom != SQ_NONE)
            {
                Piece rook = makePiece(ROOK, getColor(fromPiece));
                int castleFrom = GetIndex(dirtyMove.castleFrom, blackKingSquare, rook, false);
                int castleTo = GetIndex(dirtyMove.castleTo, blackKingSquare, rook, false);
                nnue->AddAddSubSub(acc, toIndex, castleTo, fromIndex, castleFrom);
            }
            else if (dirtyMove.capturedPiece != EMPTY)
            {
                int captureIndex = GetIndex(dirtyMove.captured, blackKingSquare, dirtyMove.capturedPiece, false);
                nnue->AddSubSub(acc, toIndex, fromIndex, captureIndex);
            }
            else
            {
                nnue->AddSub(acc, toIndex, fromIndex);
            }

            current--;
        }
    }

    accumulatorNode.isBlackComputed = true;

    bool needsFullRefreshWhite = false;

    // check if full refresh is needed
    while (lastComputedWhite >= 0 && !accumulators[lastComputedWhite].isWhiteComputed && !needsFullRefreshWhite)
    {
        needsFullRefreshWhite = accumulators[lastComputedWhite--].dirtyMove.movePiece == makePiece(KING, WHITE);
    }

    assert(lastComputedWhite >= 0);

    auto& whiteComputed = accumulators[lastComputedWhite];

    if (needsFullRefreshWhite)
    {
        board.ResetWhiteAccumulator(accumulatorNode.whiteAcc);
    }
    else
    {
        Accumulator& acc = accumulatorNode.whiteAcc;
        acc = whiteComputed.whiteAcc;


        int current = last;

        while (current != lastComputedWhite)
        {

            const auto& dirtyMove = accumulators[current].dirtyMove;
            if (dirtyMove.movePiece == EMPTY) // null-move
            {
                current--;
                continue;
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

            int fromIndex = GetIndex(dirtyMove.from, whiteKingSquare, fromPiece, true);
            int toIndex = GetIndex(dirtyMove.to, whiteKingSquare, toPiece, true);

            if (dirtyMove.castleFrom != SQ_NONE)
            {
                Piece rook = makePiece(ROOK, getColor(fromPiece));
                int castleFrom = GetIndex(dirtyMove.castleFrom, whiteKingSquare, rook, true);
                int castleTo = GetIndex(dirtyMove.castleTo, whiteKingSquare, rook, true);
                nnue->AddAddSubSub(acc, toIndex, castleTo, fromIndex, castleFrom);
            }
            else if (dirtyMove.capturedPiece != EMPTY)
            {
                int captureIndex = GetIndex(dirtyMove.captured, whiteKingSquare, dirtyMove.capturedPiece, true);
                nnue->AddSubSub(acc, toIndex, fromIndex, captureIndex);
            }
            else
            {
                nnue->AddSub(acc, toIndex, fromIndex);
            }

            current--;
        }
    }

    accumulatorNode.isWhiteComputed = true;

#ifdef VERIFY_ACCUMULATOR
    Accumulator refWhite, refBlack;
    board.ResetWhiteAccumulator(refWhite);
    board.ResetBlackAccumulator(refBlack);
    assert(std::memcmp(refWhite.data, accumulatorNode.whiteAcc.data, sizeof(refWhite.data)) == 0);
    assert(std::memcmp(refBlack.data, accumulatorNode.blackAcc.data, sizeof(refBlack.data)) == 0);
    assert(std::memcmp(refWhite.psqt, accumulatorNode.whiteAcc.psqt, sizeof(refWhite.psqt)) == 0);
    assert(std::memcmp(refBlack.psqt, accumulatorNode.blackAcc.psqt, sizeof(refBlack.psqt)) == 0);
#endif
}