#ifndef SEARCH_INFO_H
#define SEARCH_INFO_H

#include "SearchNode.h"

#define SEARCHINFO

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
    // Testing/Diagnostics
    unsigned long long numNodes;
    unsigned long long numQNodes;

    unsigned long long numQSearchBetaCutoffs;
    unsigned long long numSearchBetaCutoffs;
    unsigned long long numSearchBetaCutoffMove[10];

    unsigned long long numLMRReduced;
    unsigned long long numLMRFailHigh;
    unsigned long long numLMRFailLow;
    unsigned long long numLMRReSearch;
    unsigned long long numLMRReducts[5];

    unsigned long long numPVSZeroWindows;
    unsigned long long numPVSFailLow;
    unsigned long long numPVSFailHigh;
    unsigned long long numPVSReSearch;

    unsigned long long ttSearchHits;
    unsigned long long ttQSearchHits;
    unsigned long long ttSearchCuts;
    unsigned long long ttQSearchCuts;

    unsigned long long pvHits;        // first move is the best
    unsigned long long orderingNodes; // nodes with at least two moves (where ordering is done)

    unsigned long long startTime;

    uint8_t seldepth;

    RootMoveList rootMoves;
    RootMove bestmove;
    PVLine pv;
};

inline void PrintDebugInfo(SearchInfo& info)
{
    std::cout << "\nTotal Nodes Searched - " << info.numNodes + info.numQNodes;
    std::cout << "\n\n----QSearch----\n";
    std::cout << "\nSearched - " << info.numQNodes;
    std::cout << "\nBeta cuts - " << info.numQSearchBetaCutoffs;
    std::cout << "\nTT hits - " << info.ttQSearchHits;
    std::cout << "\nTT cuts - " << info.ttQSearchCuts;
    std::cout << "\n\n----Search----\n";
    std::cout << "\nSearched - " << info.numNodes;
    std::cout << "\n\n--Beta cuts--\n";

    for (size_t i = 0ull; i < sizeof(info.numSearchBetaCutoffMove) / sizeof(info.numSearchBetaCutoffMove[0]); i++)
    {
        if (i < sizeof(info.numSearchBetaCutoffMove) / sizeof(info.numSearchBetaCutoffMove[0]) - 1ull)
        {
            std::cout << "\nMove " << i << " - " << info.numSearchBetaCutoffMove[i];
        }
        else
        {
            std::cout << "\nMove " << i << "+ - " << info.numSearchBetaCutoffMove[i];
        }
    }
    std::cout << "\nTotal Beta-Cutoffs - " << info.numSearchBetaCutoffs;

    std::cout << "\n\nTT hits - " << info.ttSearchHits;
    std::cout << "\nTT cuts - " << info.ttSearchCuts;
    std::cout << "\nPVS Null Windows - " << info.numPVSZeroWindows;
    std::cout << "\nPVS Fail Highs - " << info.numPVSFailHigh;
    std::cout << "\nPVS Fail Lows - " << info.numPVSFailLow;
    std::cout << "\nPVS Re-Searches - " << info.numPVSReSearch;
    std::cout << "\nLMR Fail highs - " << info.numLMRFailHigh;
    std::cout << "\nLMR Fail lows - " << info.numLMRFailLow;
    std::cout << "\nLMR Re-Searches - " << info.numLMRReSearch;
    std::cout << "\nLMR Reductions - " << info.numLMRReduced;
    std::cout << "\nLMR R=1  - " << info.numLMRReducts[0];
    std::cout << "\nLMR R=2  - " << info.numLMRReducts[1];
    std::cout << "\nLMR R=3  - " << info.numLMRReducts[2];
    std::cout << "\nLMR R=4  - " << info.numLMRReducts[3];
    std::cout << "\nLMR R=5+ - " << info.numLMRReducts[4];
    std::cout << "\nPV Hits - " << info.pvHits;
    std::cout << "\nOrder Nodes - " << info.orderingNodes;
    std::cout << "\n\n----Results----\n\n";
    std::cout << "Root Moves:\n";
    for (int i = 0; i < info.rootMoves.numRoots; i++)
    {
        auto root = info.rootMoves[i];
        std::cout << root.move.toString() << " - " << root.score << " cp\n";
    }

    std::cout << "Best Move: " << info.bestmove.move.toString() << " cp - " << info.bestmove.score << std::endl;
}

#define UPDATE_INFO_NODES(info) info.numNodes++
#define UPDATE_INFO_QNODES(info) info.numQNodes++
#define UPDATE_INFO_QNODES(info) info.numQNodes++

#ifdef SEARCHINFO

#define UPDATE_INFO_QBETACUT(info) info.numQSearchBetaCutoffs++
#define UPDATE_INFO_BETACUT(info) info.numSearchBetaCutoffs++
#define UPDATE_INFO_BETACUTMOVE(info, move) info.numSearchBetaCutoffMove[std::min(move, 9)]++;

#define UPDATE_INFO_LMRREDUCE(info) info.numLMRReduced++
#define UPDATE_INFO_LMRFAILHIGH(info) info.numLMRFailHigh++
#define UPDATE_INFO_LMRFAILLOW(info) info.numLMRFailLow++
#define UPDATE_INFO_LMRRESEARCH(info) info.numLMRReSearch++
#define UPDATE_INFO_LMRREDUCT(info, reduct) info.numLMRReducts[std::min(reduct - 1, 4)]++

#define UPDATE_INFO_PVSZEROWINDOW(info) info.numPVSZeroWindows++
#define UPDATE_INFO_PVSFAILLOW(info) info.numPVSFailLow++
#define UPDATE_INFO_PVSFAILHIGH(info) info.numPVSFailHigh++
#define UPDATE_INFO_PVSRESEARCH(info) info.numPVSReSearch++

#define UPDATE_INFO_TTHIT(info) info.ttSearchHits++
#define UPDATE_INFO_TTQHIT(info) info.ttQSearchHits++
#define UPDATE_INFO_TTCUT(info) info.ttSearchCuts++
#define UPDATE_INFO_TTQCUT(info) info.ttQSearchCuts++

#define UPDATE_INFO_PVHIT(info) info.pvHits++
#define UPDATE_INFO_ORDERHIT(info) info.orderingNodes++

#else

#define UPDATE_INFO_QBETACUT(info)
#define UPDATE_INFO_BETACUT(info)
#define UPDATE_INFO_BETACUTMOVE(info, move)

#define UPDATE_INFO_LMRREDUCE(info)
#define UPDATE_INFO_LMRFAILHIGH(info)
#define UPDATE_INFO_LMRFAILLOW(info)
#define UPDATE_INFO_LMRRESEARCH(info)
#define UPDATE_INFO_LMRREDUCT(info, reduct)

#define UPDATE_INFO_PVSZEROWINDOW(info)
#define UPDATE_INFO_PVSFAILLOW(info)
#define UPDATE_INFO_PVSFAILHIGH(info)
#define UPDATE_INFO_PVSRESEARCH(info)

#define UPDATE_INFO_TTHIT(info)
#define UPDATE_INFO_TTQHIT(info)
#define UPDATE_INFO_TTCUT(info)
#define UPDATE_INFO_TTQCUT(info)

#define UPDATE_INFO_PVHIT(info)
#define UPDATE_INFO_ORDERHIT(info)

#endif

#endif