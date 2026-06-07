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