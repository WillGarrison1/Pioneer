#include "transposition.h"
#include "random.h"
#include <cstring>
#include <iostream>

Key boardHashes[64][(KING | BLACK) + 1];
Key isBlackHash;
Key castleRightsHash[16];
Key enPassantHash[8];

TranspositionTable::TranspositionTable(unsigned long long size)
{
    assert(size != 0);
    this->numBuckets = 1ULL << (63 - __builtin_clzll(size / sizeof(TranspositionBucket))); // round down to nearest power of two
    std::cout << "Num Buckets: " << this->numBuckets << std::endl;

    this->buckets = new TranspositionBucket[numBuckets];

    memset(buckets, 0, numBuckets * sizeof(TranspositionBucket));
}

TranspositionTable::~TranspositionTable()
{
    delete[] buckets;
}

TranspositionEntry *TranspositionTable::GetEntry(Key key)
{
    unsigned long long index = key & (this->numBuckets - 1);
    TranspositionBucket *bucket = &this->buckets[index];

    for (TranspositionEntry &entry : bucket->entries)
        if (entry.key == key >> 32)
            return &entry;
    return nullptr;
}

void TranspositionTable::SetEntry(Key zobrist, Score score, unsigned char depth, NodeType nodeType, unsigned char ply,
                                  Move bestMove)
{
    unsigned long long index = zobrist & (this->numBuckets - 1); // much faster than modulo
    TranspositionBucket *bucket = &this->buckets[index];

    TranspositionEntry *entry = nullptr;
    int maxPoints = INT_MIN;

    for (TranspositionEntry &e : bucket->entries)
    {
        if (!e.key) // just fill entry if empty
        {
            entry = &e;
            break;
        }

        int points = 0;
        if (e.age < ply)
            points += 4;           // punish for being older
        points += depth - e.depth; // punish for having a lower depth (higher points = worse)

        if (maxPoints < points)
        {
            entry = &e;
            maxPoints = points;
        }
    }

    entry->key = zobrist >> 32;
    entry->score = score;
    entry->depth = depth;
    entry->nodeType = nodeType;
    entry->age = ply;
    entry->move = bestMove;
}

float TranspositionTable::GetFull()
{
    unsigned long long valid = 0;
    for (unsigned long long i = 0; i < this->numBuckets; i++)
    {
        for (TranspositionEntry &e : buckets[i].entries)
            if (e.key)
                valid++;
    }

    return (float)valid / ((float)numBuckets * BUCKET_SIZE);
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