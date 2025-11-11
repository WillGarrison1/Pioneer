#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include "move.h"
#include "types.h"

#define BUCKET_SIZE 3

enum class NodeType : unsigned char
{
    Exact,
    Upper,
    Lower
};

struct TranspositionEntry
{
    unsigned int key; // the upper 16 bits of the zobrist hash
    Score score;      // The score of this position at depth
    Move move;        // best move to be played

    NodeType nodeType;   // the bounds of this positions score
    unsigned char depth; // the depth the score was calculated at
    unsigned char age;   // the ply at which this position is
};

struct TranspositionBucket
{
    TranspositionEntry entries[BUCKET_SIZE];
};

class TranspositionTable
{
public:
    /**
     * @brief Construct a new Transposition Table object
     *
     * @param size the size in bytes of the transposition table entry array
     */
    TranspositionTable(unsigned long long size);

    ~TranspositionTable();

    TranspositionEntry *GetEntry(Key key);

    void SetEntry(Key zobrist, Score score, unsigned char depth, NodeType nodeType, unsigned char ply, Move bestMove);

    float GetFull(); // gets how full the table is, from 0-1

private:
    TranspositionBucket *buckets;
    unsigned long long numBuckets;
};

extern Key boardHashes[64][(KING | BLACK) + 1];
extern Key isBlackHash;
extern Key castleRightsHash[16];
extern Key enPassantHash[8];

extern void InitZobrist();

#endif