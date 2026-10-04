#ifndef MOVE_SORT_H
#define MOVE_SORT_H

#include "PieceTable.h"
#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "move.h"

#if TUNING
#include "tuning_params.h"
#else
#include "searchParams.h"
#endif

#include <algorithm>

#define CONTINUATION_HISTORY_SIZE 3

struct MoveVal
{
    Move m;
    int16_t score;
};

enum SortType
{
    NORMAL,
    QUIESCENCE
};

MoveVal ScoreMove(const Board& board, Move m, int ply);
MoveVal ScoreMoveQ(const Board& board, Move m);

extern Move killerMoves[MAX_PLY][2];                    // each ply can have two killer moves
extern int16_t moveHistory[2][64][64];                  // History for [isWhite][from][to]
extern int16_t captureHistory[64][64][PieceType::KING]; // same as moveHistory
extern Move counterMove[64][64];
extern int16_t continuationHistory[CONTINUATION_HISTORY_SIZE][6][64][6][64];

// `ply` is the SEARCH ply, not board.getPly(). It used to be the latter, narrowed to
// `unsigned char` on the way in while ScoreMove read the table with the full-width board ply --
// so past ply 255 the write and read indices diverged and killers silently stopped working.
inline void addKillerMove(int ply, Move m)
{
    if (killerMoves[ply][0] == m)
        return;

    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = m;
}

// returns 0 if not killer move, 1 if first killer, 2 if second killer
inline int isKillerMove(int ply, Move m)
{
    if (killerMoves[ply][0] == m)
    {
        return 1;
    }
    if (killerMoves[ply][1] == m)
    {
        return 2;
    }
    return 0;
}

inline void updateContinuationHistory(Board& board, Move m, int depth, bool negate)
{
    int negative = negate ? -1 : 1;
    const BoardState* prevState = board.getState();
    PieceType moved = getType(board.getSQ(m.from()));
    int clampedBonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY) * negative;
    for (int i = 0; i < CONTINUATION_HISTORY_SIZE; i++)
    {
        if (!prevState || prevState->moved == EMPTY)
            break;

        PieceType pType = getType(prevState->moved);
        Move prevMove = prevState->move;
        continuationHistory[i][pType - 1][prevMove.to()][moved - 1][m.to()] +=
            clampedBonus -
            continuationHistory[i][pType - 1][prevMove.to()][moved - 1][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;

        prevState = prevState->prev;
    }
}

inline void addHistoryBonus(bool isWhite, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth * 50, -MAX_HISTORY, MAX_HISTORY);
    moveHistory[isWhite][m.from()][m.to()] +=
        clampedBonus - moveHistory[isWhite][m.from()][m.to()] * std::abs(clampedBonus) / MAX_HISTORY;
}

inline void addHistoryPenalty(bool isWhite, Move m, int depth)
{
    const int penalty = std::clamp(depth * depth * 50, -MAX_HISTORY, MAX_HISTORY);
    auto gravity = moveHistory[isWhite][m.from()][m.to()] * std::abs(penalty) / MAX_HISTORY;
    moveHistory[isWhite][m.from()][m.to()] -= penalty + gravity;
}

inline void addCaptureBonus(PieceType victimType, Move m, int depth)
{
    int clampedBonus = std::clamp(depth * depth * depth, -MAX_CAPTURE_HISTORY, MAX_CAPTURE_HISTORY);
    captureHistory[m.from()][m.to()][victimType - 1] +=
        clampedBonus - captureHistory[m.from()][m.to()][victimType - 1] * std::abs(clampedBonus) / MAX_CAPTURE_HISTORY;
}

inline void addCapturePenalty(PieceType victimType, Move m, int depth)
{
    const int penalty = std::clamp(depth * depth * depth, -MAX_CAPTURE_HISTORY, MAX_CAPTURE_HISTORY);
    captureHistory[m.from()][m.to()][victimType - 1] -=
        penalty + captureHistory[m.from()][m.to()][victimType - 1] * std::abs(penalty) / MAX_CAPTURE_HISTORY;
}

/**
 * @brief The piece type captured by a CAPTURE-typed move.
 *
 * En passant is encoded as a plain CAPTURE onto an empty square, so an empty destination
 * identifies it unambiguously. Testing `m.to() == board.getEnPassantSqr()` instead is wrong:
 * that is true for *any* piece landing on the en passant square, not just a pawn capturing there.
 */
inline PieceType CapturedType(const Board& board, Move m)
{
    const Piece victim = board.getSQ(m.to());
    return victim == EMPTY ? PAWN : getType(victim);
}

inline Score Mvv_Lva_Score(const Board& board, Move m)
{
    PieceType victimType = getType(board.getSQ(m.to()));
    PieceType pieceType = getType(board.getSQ(m.from()));

    return (pieceScores[victimType] - pieceScores[pieceType]);
}

struct MoveSorter
{
    enum MoveIndex
    {
        BEST,
        CAPTURE,
        QUIET
    };

    MoveVal moves[1 + 218 + 218]; // best, captures, quiets
    uint8_t numMoves[3]{0}; // best, captures, quiets

    uint8_t currentBucketNum;

    const Board& board;
    int ply;

    MoveSorter(const Board& board, MoveList* mlist, Move best, int ply) : currentBucketNum(0), board(board), ply(ply)
    {
        for (uint32_t i = 0; i < mlist->GetSize(); i++)
        {
            if (mlist->moves[i] == best)
            {
                moves[0] = {best, PV_BONUS};
                numMoves[MoveIndex::BEST]++;
            }
            else
            {
                MoveIndex bucket = mlist->moves[i].isType<MoveType::CAPTURE>() ? CAPTURE : QUIET;
                MoveVal* ptr = GetBucket(bucket);
                ptr[numMoves[bucket]++] = {mlist->moves[i],0};
            }
        }
    }
    
    ~MoveSorter() = default;

    MoveVal* GetBucket(int bucketNum)
    {
        return moves + ((bucketNum == 2) ? 219 : bucketNum);
    }

    void ScoreBucket(int bucketNum)
    {
        MoveVal* bucket = GetBucket(bucketNum);
        if (bucketNum == CAPTURE)
        {
            for (uint32_t i = 0; i < numMoves[bucketNum]; i++)
            {
                bucket[i] = ScoreMoveQ(board, bucket[i].m);
            }
        }
        else if (bucketNum == QUIET)
        {
            for (uint32_t i = 0; i < numMoves[bucketNum]; i++)
            {
                bucket[i] = ScoreMove(board, bucket[i].m, ply);
            }
        }
    }

    Move Next()
    {
        while (numMoves[currentBucketNum] == 0)
        {
            currentBucketNum += 1; // if bucket empty, go to next bucket
            if (currentBucketNum >= std::size(numMoves))
                return 0; // no more buckets, return invalid move to signal end
            ScoreBucket(currentBucketNum);
        }

        MoveVal* currentPtr = GetBucket(currentBucketNum);

        int bestIndex = 0;
        int bestScore = currentPtr[0].score;

        for (int i = 1; i < numMoves[currentBucketNum]; i++)
        {
            if (bestScore < currentPtr[i].score)
            {
                bestIndex = i;
                bestScore = currentPtr[i].score;
            }
        }

        MoveVal bestmove = currentPtr[bestIndex];

        // Swap the best move with the end list
        currentPtr[bestIndex] = currentPtr[--numMoves[currentBucketNum]];
        currentPtr[numMoves[currentBucketNum]] = bestmove;

        return bestmove.m;
    }
};

#endif