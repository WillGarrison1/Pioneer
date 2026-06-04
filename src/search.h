#ifndef SEARCH_H
#define SEARCH_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "SearchNode.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"
#include "types.h"

enum NodeType
{
    PVNode,
    CUTNode,
    RootNode
};

struct RootMove
{
    Move move;
    Score score;
};

struct RootMoveList
{
    RootMove rootMoves[256];
    int numRoots;

    inline RootMove& operator[](int index)
    {
        assert(index < numRoots);
        return rootMoves[index];
    }

    void Add(RootMove rootMove)
    {
        rootMoves[numRoots++] = rootMove;
    }

    void Clear()
    {
        numRoots = 0;
    }
};

struct SearchInfo
{
    unsigned long long numNodes;
    unsigned long long numQNodes;
    unsigned long long numBetaCutoffs;
    unsigned long long ttHits;
    unsigned long long pvHits;
    unsigned long long orderingNodes;
    unsigned long long startTime;

    RootMoveList rootMoves;
    RootMove bestmove;
    PVLine pv;
};

struct SearchConstraints
{
    // if any of these values are 0, treat them as infinite

    // maximum depth
    unsigned int maxDepth;

    // max nodes to search
    unsigned int maxNodes;

    // max time to think (milliseconds)
    unsigned int movetime;

    // the amount of time left for the current side (milliseconds)
    unsigned int remainingTime;
};

class Searcher
{
  public:
    Searcher();
    ~Searcher();

    /**
     * @brief Starts a search for the best move and evaluation for a given position
     *
     * @param board the current position
     * @param constraints search constraints
     */
    void StartSearch(const Board& board, const SearchConstraints& constraints);

    void Stop();

    inline const SearchInfo& GetSearchInfo()
    {
        return info;
    }

    void ClearTT()
    {
        ttable.Clear();
    }

  private:
    SearchConstraints constraints;
    SearchInfo info;
    Board board;
    TranspositionTable ttable;

    std::atomic_bool isRunning;
    std::atomic_bool isQuit;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread thread;

    void WorkerLoop();
    void DoSearch();
    void ComputeMovetime();
    void IterativeDeepening(Board& board);

    template <NodeType nodeT>
    Score Search(Board& board, int depth, int ply, Score alpha, Score beta, SearchNode* node,
                 const bool nullMoveAllowed = true);
    Score QSearch(Board& board, int ply, Score alpha, Score beta, SearchNode* node);
};

#endif