#include "transposition.h"
#include "random.h"
#include <cstring>

Key boardHashes[64][(KING | BLACK) + 1];
Key isBlackHash;
Key castleRightsHash[16];
Key enPassantHash[8];

TranspositionTable::TranspositionTable(unsigned long long size)
{
    this->numEntries = size / sizeof(TranspositionEntry);
    this->entries = new TranspositionEntry[numEntries];

    memset(entries, 0, numEntries * sizeof(TranspositionEntry));
}

TranspositionTable::~TranspositionTable()
{
    delete[] entries;
}

TranspositionEntry* TranspositionTable::GetEntry(Key key)
{
    unsigned long long index = key & (this->numEntries - 1);
    TranspositionEntry* entry = &this->entries[index];
    if (entry->key == key >> 48)
        return entry;
    return nullptr;
}

void TranspositionTable::SetEntry(Key zobrist, Score score, unsigned char depth, NodeType nodeType, unsigned char ply,
                                  Move bestMove)
{
    unsigned long long index = zobrist & (this->numEntries - 1);
    TranspositionEntry* entry = &this->entries[index];
    entry->key = zobrist >> 48;
    entry->score = score;
    entry->depth = depth;
    entry->nodeType = nodeType;
    entry->age = ply;
    entry->move = bestMove;
}

float TranspositionTable::GetFull()
{
    unsigned long long valid = 0;
    for (unsigned long long i = 0; i < this->numEntries; i++)
    {
        if (entries[i].key)
            valid++;
    }

    return (float)valid / (float)numEntries;
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