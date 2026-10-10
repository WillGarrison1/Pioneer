#include <chrono>
#include <iostream>

#include "MoveSort.h"
#include "direction.h"
#include "engine.h"
#include "evaluate.h"
#include "move.h"
#include "movegen.h"
#include "nnue/nnue.h"
#include "perft.h"
#include "platform.h"
#include "search.h"
#include "square.h"
#include "time.h"
#include "transposition.h"
#include <cstring>

Engine::Engine()
{
    initSquare();
    initDirection();
    initBBs();
    InitZobrist();
    InitMagics();

    std::memset(moveHistory, 0, sizeof(moveHistory));

    std::string exeDir;
    GetExecutablePath(exeDir);
    exeDir = exeDir.substr(0, exeDir.find_last_of("/\\"));
    bool nnueLoaded = nnue->Load(exeDir + "/nnue_bin/nnue02.bin");
    if (!nnueLoaded)
    {
        std::cerr << "Failed to load NNUE network." << std::endl;
    }

    board = new Board;
    board->setFen(START_FEN, &states[0]);

    searcher = new Searcher;
}

Engine::~Engine()
{
    delete board;
    delete searcher;
}

void Engine::makemove(Move move)
{
    quiesceSearch();

    MoveList legal;
    board->generateMoves<ALL_MOVES>(&legal);

    DirtyMove dirtyMove;
    for (Move* mPtr = legal.moves; mPtr < legal.end; mPtr++)
    {
        Move m = *mPtr;
        if (m.to() == move.to() && m.from() == move.from() &&
            ((m.type() == PROMOTION && move.promotion() == m.promotion()) || (m.type() != PROMOTION)))
        {
            board->makeMove(m, &states[board->getPly() + 1], dirtyMove);
            break;
        }
    }
}

void Engine::go(const GoParams& params)
{
    SearchConstraints constraints{};
    constraints.maxDepth = params.depth;
    constraints.maxNodes = params.nodes;
    constraints.movetime = params.movetime;
    constraints.remainingTime = board->whiteToMove ? params.wtime : params.btime;
    constraints.increment = board->whiteToMove ? params.winc : params.binc;
    constraints.movesToGo = params.movestogo;
    constraints.infinite = params.infinite;
    constraints.quiet = false;
    searcher->StartSearch(*board, constraints);
}

void Engine::SetHashSize(unsigned int megabytes)
{
    if (megabytes == 0)
        return;
    searcher->ResizeTT(static_cast<unsigned long long>(megabytes) * 1024ULL * 1024ULL);
}

// Standard bench set. Kept fixed on purpose: the node total is only meaningful as a regression
// signal if the positions never change.
static const char* BENCH_FENS[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "2rq1rk1/pp1bppbp/3p1np1/8/3NP3/1BN1BP2/PPPQ2PP/2KR3R w - - 0 1",
    "4rrk1/pp1n1pp1/2pb1q1p/3p4/3P1B2/2PB1N2/PPQ2PPP/R4RK1 w - - 0 1",
    "8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1",
    "6k1/5ppp/8/8/8/8/5PPP/2R3K1 w - - 0 1",
    "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1",
    "r1bqkb1r/pp3ppp/2n1pn2/2pp4/3P1B2/2PBP3/PP1N1PPP/R2QK1NR w KQkq - 0 1",
};

void Engine::bench(unsigned int depth)
{
    quiesceSearch();

    if (depth == 0)
        depth = 12;

    unsigned long long totalNodes = 0;
    const unsigned long long start = getTime();

    for (const char* fen : BENCH_FENS)
    {
        // Clear between positions so the total does not depend on carry-over TT state.
        searcher->ClearTT();
        board->setFen(fen, &states[0]);

        SearchConstraints constraints{};
        constraints.maxDepth = depth;
        constraints.quiet = true;

        searcher->StartSearch(*board, constraints);
        searcher->WaitForSearch();

        const SearchInfo& info = searcher->GetSearchInfo();
        totalNodes += info.numNodes + info.numQNodes;
    }

    const unsigned long long elapsed = getTime() - start;
    const unsigned long long nps = totalNodes * 1000ULL / std::max(elapsed, 1ULL);

    // Trailing format is what OpenBench parses.
    std::cout << "Bench depth " << depth << " time " << elapsed << " ms\n";
    std::cout << totalNodes << " nodes " << nps << " nps" << std::endl;

    board->setFen(START_FEN, &states[0]);
}

// Standard perft positions with published node counts.
struct PerftCase
{ 
    const char* fen;
    unsigned int depth;
    unsigned long long expected;
};

static const PerftCase PERFT_CASES[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609ULL},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603ULL},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083ULL},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292ULL},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194ULL},
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551ULL},
    {"r3k2r/1K6/8/8/8/8/8/8 w kq - 0 1", 5, 53062ULL},
};

bool Engine::perftSuite()
{
    quiesceSearch();

    bool allPassed = true;
    const unsigned long long start = getTime();

    for (const PerftCase& c : PERFT_CASES)
    {
        board->setFen(c.fen, &states[0]);
        const unsigned long long got = perft(*board, c.depth);
        const bool ok = (got == c.expected);
        allPassed &= ok;

        std::cout << (ok ? "PASS " : "FAIL ") << "depth " << c.depth << "  got " << got << "  expected " << c.expected
                  << "  " << c.fen << "\n";
    }

    std::cout << (allPassed ? "perftsuite: ALL PASSED" : "perftsuite: FAILURES PRESENT") << " ("
              << (getTime() - start) << " ms)" << std::endl;

    board->setFen(START_FEN, &states[0]);
    return allPassed;
}

void Engine::stop()
{
    searcher->Stop();
}

void Engine::goPerft(unsigned int depth)
{
    quiesceSearch();

    unsigned long long start = getTime();
    unsigned long long moveCount = 0;

    MoveList moves;
    board->generateMoves<ALL_MOVES>(&moves);
    BoardState state;
    DirtyMove dirtyMove;

    for (Move* mPtr = moves.moves; mPtr < moves.end; mPtr++)
    {
        Move move = *mPtr;
        board->makeMove(move, &state, dirtyMove);
        unsigned long long count = perft(*board, depth - 1);
        board->undoMove();

        moveCount += count;

        std::cout << move.toString() << ": " << count << "\n";
    }

    unsigned long long end = getTime();
    std::cout << "Total Moves: " << moveCount << " Took: " << (end - start) << " ms" << std::endl;
}

void Engine::eval()
{
    quiesceSearch();

    Accumulator white, black;
    board->ResetWhiteAccumulator(white);
    board->ResetBlackAccumulator(black);
    Accumulator& us = board->whiteToMove ? white : black;
    Accumulator& them = board->whiteToMove ? black : white;
    float score = nnue->Evaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);
    float psqt = nnue->FastEvaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);

    for (Rank r = RANK_8; r >= RANK_1; r = (Rank)(r - 1))
    {
        std::string rank[5] = {"+", "|", "|", "|", "|"};
        for (File f = FILE_A; f <= FILE_H; f = (File)(f + 1))
        {
            Square s = getSquare(f, r);
            Piece p = board->getSQ(s);

            if (p == EMPTY)
            {
                rank[0] += "-------+";
                rank[1] += "       |";
                rank[2] += "       |";
                rank[3] += "       |";
                rank[4] += "       |";
                continue;
            }

            float pieceVal = 0.0f;
            if (getType(p) != KING)
            {
                board->removePiece(s);

                board->ResetWhiteAccumulator(white);
                board->ResetBlackAccumulator(black);
                float newScore = nnue->Evaluate(*board, us, them) * (board->whiteToMove ? 1.0f : -1.0f);
                pieceVal = (score - newScore) / 100;

                board->addPiece(p, s);
            }

            char buffer[7];
            std::snprintf(buffer, sizeof(buffer), "%.1f", pieceVal);

            std::string value(buffer);
            if (value.length() < 7)
            {
                int dif = 7 - value.length();
                int leftZ = std::ceil(dif / 2.0f);
                int rightZ = std::floor(dif / 2.0f);
                value.insert(value.begin(), leftZ, ' ');
                value.insert(value.end(), rightZ, ' ');
            }

            rank[0] += "-------+";
            rank[1] += "       |";
            rank[2] += "   "+pieceToString(p)+"   |";
            rank[3] += value+"|";
            rank[4] += "       |";
        }
        std::cout << rank[0] << "\n" << rank[1] << "\n" << rank[2] << "\n" << rank[3] << "\n" << rank[4] << "\n";
    }
    std::cout << "+-------+-------+-------+-------+-------+-------+-------+-------+\n\n";
    std::cout << "Positional: " << score - psqt << "\nPsqt: " << psqt << "\n\nEval: " << score << std::endl;
}

void Engine::isCheck(Move move)
{
    MoveList legal;
    board->generateMoves<ALL_MOVES>(&legal);

    for (Move* mPtr = legal.moves; mPtr < legal.end; mPtr++)
    {
        Move m = *mPtr;
        // Match the way makemove() does. Comparing promotion() unconditionally silently failed
        // for castling: the generated castle move encodes EMPTY in the promotion bits, which
        // reads back as QUEEN, so `check e1g1` matched nothing and printed nothing at all.
        if (m.to() == move.to() && m.from() == move.from() &&
            ((m.type() == PROMOTION && move.promotion() == m.promotion()) || (m.type() != PROMOTION)))
        {
            std::cout << board->isCheckMove(m) << std::endl;
            return;
        }
    }
}