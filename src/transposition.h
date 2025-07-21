#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include "move.h"
#include "types.h"

enum class NodeType : unsigned char
{
    Exact,
    Upper,
    Lower
};

struct TranspositionEntry
{
    unsigned short key;  // the upper 16 bits of the zobrist hash
    unsigned char depth; // the depth the score was calculated at
    unsigned char age;   // the ply at which this position is
    Score score;         // The score of this position at depth
    NodeType nodeType;   // the bounds of this positions score
    Move move;           // best move to be played
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

    TranspositionEntry* GetEntry(Key key);

    void SetEntry(Key zobrist, Score score, unsigned char depth, NodeType nodeType, unsigned char ply, Move bestMove);

    float GetFull(); // gets how full the table is, from 0-1

  private:
    TranspositionEntry* entries;
    unsigned long long numEntries;
};

extern Key boardHashes[64][(KING | BLACK) + 1];
extern Key isBlackHash;
extern Key castleRightsHash[16];
extern Key enPassantHash[8];

extern void InitZobrist();

#endif