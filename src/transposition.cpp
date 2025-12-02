#include "transposition.h"
#include "random.h"
#include <cstring>
#include <iostream>

alignas(64) Key boardHashes[64][(KING | BLACK) + 1];
Key isBlackHash;
Key castleRightsHash[16];
Key enPassantHash[8];

TranspositionTable* tTable =
    new (std::align_val_t(64)) TranspositionTable(1024 * 1024 * 64); // transposition table with a size of 64 MB

void TranspositionEntry::Set(Key key, Score score, Move move, unsigned char depth, unsigned char age, NodeBound bound)
{
    this->key = key >> 48;
    this->score = score;
    this->move = move.getMove();
    this->depth = depth;
    this->flags = ((unsigned char)bound << 6) | age;
}

TranspositionTable::TranspositionTable(unsigned long long size)
{
    assert(size != 0);
    this->numBuckets =
        1ULL << (63 - __builtin_clzll(size / sizeof(TranspositionBucket))); // round down to nearest power of two
    std::cout << "Num Buckets: " << this->numBuckets << std::endl;

    this->buckets = new (std::align_val_t(64)) TranspositionBucket[numBuckets];
    age = 0;

    memset(buckets, 0, numBuckets * sizeof(TranspositionBucket));
}

TranspositionTable::~TranspositionTable()
{
    delete[] buckets;
}

TranspositionEntry* TranspositionTable::GetEntry(Key key)
{
    unsigned long long index = key & (this->numBuckets - 1);
    TranspositionBucket* bucket = &this->buckets[index];
    unsigned short intKey = key >> 48;

    TranspositionEntry* result = nullptr;

    for (TranspositionEntry* entry = bucket->entries; entry < bucket->entries + BUCKET_SIZE; entry++)
        if (entry->key == intKey)
            result = entry;
    return result;
}

void TranspositionTable::SetEntry(Key zobrist, Score score, unsigned char depth, NodeBound bound, Move bestMove)
{
    unsigned long long index = zobrist & (this->numBuckets - 1); // much faster than modulo
    TranspositionBucket* bucket = &this->buckets[index];

    TranspositionEntry* entry = nullptr;
    int maxPoints = -__INT32_MAX__;

    for (TranspositionEntry& e : bucket->entries)
    {
        if (!e.key) // just fill entry if empty
        {
            entry = &e;
            break;
        }

        int points = 0;
        if (e.getAge() != age)
            points += 4;           // punish for being older
        points += depth - e.depth; // punish for having a lower depth (higher points = worse)

        if (e.getNodeBound() == NodeBound::Exact)
            points -= 1;
        if (bound == NodeBound::Exact)
            points += 1;

        if (maxPoints < points)
        {
            entry = &e;
            maxPoints = points;
        }
    }

    entry->Set(zobrist, score, bestMove, depth, age, bound);
}

float TranspositionTable::GetFull()
{
    unsigned long long valid = 0;
    for (unsigned long long i = 0; i < this->numBuckets; i++)
    {
        for (TranspositionEntry& e : buckets[i].entries)
            if (e.key)
                valid++;
    }

    return (float)valid / ((float)numBuckets * BUCKET_SIZE);
}

void TranspositionTable::Clear()
{
    age = 0;
    std::memset(this->buckets, 0, this->numBuckets * sizeof(TranspositionBucket));
}

void InitZobrist()
{
    for (int i = 0; i < 64; i++)
    {
        for (int p = 0; p < 15; p++)
        {
            boardHashes[i][p] = RandNum();
        }
    }

    isBlackHash = RandNum();

    for (int i = 0; i < 16; i++)
    {
        castleRightsHash[i] = RandNum();
    }

    for (int i = 0; i < 8; i++)
    {
        enPassantHash[i] = RandNum();
    }
}