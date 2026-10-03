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
#include "nnue/accumulatorList.h"
#include "transposition.h"
#include "types.h"
#include "searchInfo.h"

enum NodeType
{
    PVNode,
    CUTNode,
    RootNode
};

struct SearchConstraints
{
    // if any of these values are 0, treat them as infinite

    // maximum depth
    unsigned int maxDepth;

    // max nodes to search
    unsigned int maxNodes;

    // explicit `go movetime` in milliseconds (0 = not specified)
    unsigned int movetime;

    // the amount of time left for the current side (milliseconds)
    unsigned int remainingTime;

    // the increment gained per move for the current side (milliseconds)
    unsigned int increment;

    // moves remaining until the next time control (0 = sudden death / not specified)
    unsigned int movesToGo;

    // `go infinite` -- search until an explicit `stop`
    bool infinite;

    // suppress all per-search output (info lines, bestmove, debug dump). Used by `bench`, which
    // reports a single aggregate line of its own.
    bool quiet;
};

class Searcher
{
  public:
    Searcher();
    ~Searcher();

    void Makemove(Move m, BoardState& state, int ply);
    void Undomove(int ply);

    void MakeNullmove(BoardState& state, int ply);
    void UndoNullmove(int ply);

    /**
     * @brief Starts a search for the best move and evaluation for a given position
     *
     * @param board the current position
     * @param constraints search constraints
     */
    void StartSearch(const Board& board, const SearchConstraints& constraints);

    void Stop();

    /**
     * @brief Blocks until the in-flight search has finished.
     *
     * StartSearch is asynchronous, so callers that need the result (bench, tests) must wait
     * rather than racing the worker thread.
     */
    void WaitForSearch();

    inline const SearchInfo& GetSearchInfo()
    {
        return info;
    }

    void ClearTT()
    {
        ttable.Clear();
    }

    /**
     * @brief Resizes the transposition table to (at most) `bytes`, rounded down so the bucket
     * count stays a power of two -- index math relies on `key & (numBuckets - 1)`.
     */
    void ResizeTT(unsigned long long bytes);

  private:
    SearchConstraints constraints;

    // Computed once per search by ComputeMovetime().
    //   soft: don't *start* another iterative-deepening iteration past this point.
    //   hard: abandon the search mid-iteration at this point.
    // Splitting the two is what lets the engine spend its increment: a search may run well past
    // soft finishing the iteration it is in, but never past hard.
    unsigned long long softLimit;
    unsigned long long hardLimit;

    SearchInfo info;
    Board board;
    TranspositionTable ttable;
    AccumulatorList accumulators;

    std::atomic_bool isRunning;
    std::atomic_bool isSearching;
    std::atomic_bool isQuit;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread thread;

    void WorkerLoop();
    void DoSearch();
    void ComputeMovetime();
    void IterativeDeepening(Board& board);

    template <NodeType nodeT>
    Score Search(int depth, int ply, Score alpha, Score beta, SearchNode* node, const bool nullMoveAllowed = true);
    Score QSearch(int ply, Score alpha, Score beta, SearchNode* node);
};

#endif