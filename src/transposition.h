#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <cstdint>

#include "move.h"
#include "types.h"

#define BUCKET_SIZE 3

enum class NodeBound : unsigned char
{
    Exact,
    Upper,
    Lower
};

struct TranspositionEntry
{
    uint32_t key;  // the upper 32 bits of the zobrist hash
    int16_t score; // The score of this position at depth
    uint16_t move; // best move to be played

    unsigned char depth; // the depth the score was calculated at
    unsigned char flags; // the age at which this position is and the node bound
                         // format: nodebound << 6 | age
    void Set(Key key, Score score, Move move, unsigned char depth, unsigned char age, NodeBound bound);

    inline NodeBound getNodeBound() const
    {
        return (NodeBound)(flags >> 6);
    }

    inline unsigned char getAge() const
    {
        return flags & 0x3f;
    }
} __attribute__((packed));

struct TranspositionBucket
{
    TranspositionEntry entries[BUCKET_SIZE];
    char padding[2]; // pad to 32 bytes
};

static_assert(sizeof(TranspositionEntry) == 10, "TranspositionEntry is not 10 bytes!");
static_assert(sizeof(TranspositionBucket) == 32, "TranspositionBucket is not 32 bytes!");
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

    inline void IncrementAge()
    {
        age = (age + 1) & 0x3f;
    };

    TranspositionEntry* GetEntry(Key key);

    void SetEntry(Key zobrist, Score score, unsigned char depth, NodeBound bound, Move bestMove);

    float GetFull(); // gets how full the table is, from 0-1

    void Clear();

  private:
    TranspositionBucket* buckets;
    unsigned char age;
    unsigned long long numBuckets;
};

extern TranspositionTable* tTable;

extern Key boardHashes[64][(KING | BLACK) + 1];
extern Key isBlackHash;
extern Key castleRightsHash[16];
extern Key enPassantHash[8];

extern void InitZobrist();

#endif