#include "transposition.h"
#include "random.h"
#include <cmath>
#include <cstring>
#include <iostream>

alignas(64) Key boardHashes[64][(KING | BLACK) + 1];
Key isBlackHash;
Key castleRightsHash[16];
Key enPassantHash[8];

void TranspositionEntry::Set(Key key, Score score, Move move, unsigned char depth, unsigned char age, NodeBound bound)
{
    this->key = key >> 32ULL;
    this->score = (int16_t)score;
    this->move = move.getMove();
    this->depth = depth;
    this->flags = ((unsigned char)bound << 6) | age;
}

TranspositionTable::TranspositionTable(unsigned long kbytes) : buckets(nullptr)
{
    assert(kbytes != 0);

    // round down to nearest power of two
    unsigned long long bytes = (1ULL << static_cast<unsigned long long>(std::floorl(std::log2l(kbytes)))) * 1024ULL;
    Resize(bytes);
}

TranspositionTable::~TranspositionTable()
{
    operator delete[](this->buckets, std::align_val_t(64));
}

void TranspositionTable::Resize(unsigned long long bytes)
{
    numBuckets = bytes / sizeof(TranspositionBucket);
    std::cout << "Num Buckets: " << this->numBuckets << " (" << bytes << " bytes)" << std::endl;

    if (this->buckets)
    {
        operator delete[](this->buckets, std::align_val_t(64));
    }

    this->buckets = new (std::align_val_t(64)) TranspositionBucket[numBuckets];
    age = 0;

    memset(buckets, 0, numBuckets * sizeof(TranspositionBucket));
}

TranspositionEntry* TranspositionTable::GetEntry(Key key)
{
    unsigned long long index = key & (this->numBuckets - 1);
    TranspositionBucket* bucket = &this->buckets[index];
    uint32_t intKey = key >> 32ULL;

    for (TranspositionEntry* entry = bucket->entries; entry < bucket->entries + BUCKET_SIZE; entry++)
    {
        if (entry->key == intKey)
        {
            return entry;
        }
    }

    return nullptr;
}

void TranspositionTable::SetEntry(Key zobrist, Score score, int depth, NodeBound bound, Move bestMove)
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

        if (e.key == zobrist >> 32ULL) // if we find the position in the table
        {
            e.setAge(age);
            if (depth < e.depth)
                return;
            if (e.getNodeBound() == NodeBound::Exact && bound != NodeBound::Exact)
                return;
            entry = &e;
            break;
        }

        int points = 0;
        if (e.getAge() != age)
            points += 4;                // punish for being older
        points += depth - (int)e.depth; // punish for having a lower depth (higher points = worse)

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

    entry->Set(zobrist, score, bestMove, (unsigned char)depth, age, bound);
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

void TranspositionTable::Prefetch(Key zobrist)
{
    unsigned long long index = zobrist & (this->numBuckets - 1); // much faster than modulo
    __builtin_prefetch(&this->buckets[index], 0, 1);
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