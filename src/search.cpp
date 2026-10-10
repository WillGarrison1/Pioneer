#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstring>
#include <format>
#include <iostream>

#include "search.h"

#include "MoveSort.h"
#include "SearchNode.h"
#include "evaluate.h"
#include "profile.h"
#include "time.h"
#include "transposition.h"

#ifdef TUNING
#include "tuning_params.h"
#else
#include "searchParams.h"
#endif

#define INF 32000
#define MATE 31000

#define FUTILITY_MARGIN(DEPTH) (FUTILITY_OFFSET + FUTILITY_MULTI * (DEPTH))
#define FUTILITY_DEPTH 4

// Hard ceiling on search ply. `ply` is NOT bounded by `depth`: a check extension keeps depth
// constant while ply still increments, so a forcing sequence can climb indefinitely. The
// accumulator array holds exactly MAX_DEPTH nodes and Makemove() writes index ply+1, so without
// this cap a long check sequence writes past the end of that allocation. Leave headroom for the
// qsearch that runs on top of the deepest main-search ply.
constexpr int PLY_LIMIT = MAX_DEPTH - 8;

#define NULL_DEPTH 3
#define IIR_DEPTH 3 // internal iterative reduction depth
#define DELTA 200

constexpr auto lmrTable = [] {
    std::array<std::array<int, 256>, MAX_DEPTH> table{};
    for (int d = 0; d < MAX_DEPTH; d++)
    {
        for (int m = 0; m < 256; m++)
        {
            table[d][m] = LMR_OFFSET + std::log(d > 0 ? d : 1) * std::log(m > 0 ? m : 1) / LMR_DIVISOR;
        }
    }
    return table;
}();

inline bool isWin(Score s)
{
    return s > MATE - MAX_DEPTH;
}

inline bool isLoss(Score s)
{
    return s < -MATE + MAX_DEPTH;
}

inline Score mateToTT(Score s, unsigned char ply)
{
    return isWin(s) ? s + ply : (isLoss(s) ? s - ply : s);
}

inline Score ttToMate(Score s, unsigned char ply)
{
    return isWin(s) ? s - ply : (isLoss(s) ? s + ply : s);
}

/**
 * @brief Formats a score as a UCI `score` field.
 *
 * Mate scores were previously reported as raw centipawns (e.g. "score cp 30995"), which no GUI or
 * tournament manager interprets as a mate. UCI wants "score mate <N>", N in *moves*, negative
 * when we are the one getting mated.
 */
std::string ScoreToUCI(Score s)
{
    if (isWin(s))
        return "score mate " + std::to_string((MATE - s + 1) / 2);
    if (isLoss(s))
        return "score mate " + std::to_string(-((MATE + s + 1) / 2));

    return "score cp " + std::to_string(s);
}

void UpdatePV(PVLine* out, Move move, const PVLine* childLine)
{
    out->moves[0] = move;
    uint8_t n = std::min(childLine->len, static_cast<uint8_t>(MAX_DEPTH - 2));
    for (uint8_t i = 0; i < n; i++)
        out->moves[i + 1] = childLine->moves[i];
    out->len = n + 1;
}

/**
 * @brief Calculates the depth reduction for LMR
 *
 * @param depth the current depth
 * @param moveNum the current move number
 * @return constexpr int
 */
constexpr int LMRReduction(int depth, int moveNum)
{
    return lmrTable[depth][moveNum];
}

std::string GetMoveListString(PVLine* l)
{
    std::string moves;
    for (unsigned int i = 0; i < l->len; i++)
    {
        moves += l->moves[i].toString() + " ";
    }

    return moves;
}

Searcher::Searcher()
    : ttable(64 * 1024), isRunning(false), isSearching(false), isQuit(false),
      thread(std::thread([this] { WorkerLoop(); }))
{
}

Searcher::~Searcher()
{
    Stop();

    {
        std::lock_guard lock(mtx);
        isQuit = true;
    }
    cv.notify_all();

    if (thread.joinable())
        thread.join();
}

void Searcher::Makemove(Move m, BoardState& state, int ply)
{
    accumulators.SetCurrent(ply + 1);
    accumulators.Current().isWhiteComputed = false;
    accumulators.Current().isBlackComputed = false;
    accumulators.Current().dirtyMove = {};
    board.makeMove(m, &state, accumulators.Current().dirtyMove);
}

void Searcher::Undomove(int ply)
{
    accumulators.SetCurrent(ply);
    board.undoMove();
}

void Searcher::MakeNullmove(BoardState& state, int ply)
{
    accumulators.SetCurrent(ply + 1);
    accumulators.Current().isWhiteComputed = false;
    accumulators.Current().isBlackComputed = false;
    accumulators.Current().dirtyMove = {};
    board.makeNullMove(&state);
}

void Searcher::UndoNullmove(int ply)
{
    accumulators.SetCurrent(ply);
    board.undoNullMove();
}

Score Searcher::QSearch(int ply, Score alpha, Score beta, SearchNode* node)
{
    PROFILE_FUNC();
    UPDATE_INFO_QNODES(info);

    if (!isRunning.load(std::memory_order::memory_order_relaxed))
        return 0;

    // QSearch recurses into itself and never re-enters Search, so these have to be repeated here.
    // The draw checks in particular matter: when in check we generate ALL_MOVES (including quiets),
    // so a perpetual-check sequence has no other terminating condition.
    if (board.getState()->repetition >= 3)
        return 0;

    // See PLY_LIMIT: nothing else bounds `ply`, and exceeding it is an out-of-bounds write.
    if (ply >= PLY_LIMIT)
        return board.getNumChecks() ? 0 : Eval<FULL>(board, accumulators);

    TranspositionEntry* entry = ttable.GetEntry(board.getHash());
    Move bestEntryMove = 0;
    if (entry)
    {
        UPDATE_INFO_TTQHIT(info);

        Score corrected = ttToMate(entry->score, ply);
        if (entry->getNodeBound() == NodeBound::Exact ||
            (entry->getNodeBound() == NodeBound::Upper && corrected <= alpha) ||
            (entry->getNodeBound() == NodeBound::Lower && corrected >= beta))
        {
            UPDATE_INFO_TTQCUT(info);
            entry->setAge(ttable.GetAge()); // reset the age for this node
            return corrected;
        }

        bestEntryMove = entry->move;
    }

    Score originalAlpha = alpha;
    Score pat = 0;

    // Generate moves

    MoveList moves;
    if (!board.getNumChecks()) // If not in check, generate captures
    {
        pat = Eval<FULL>(board, accumulators);

        if (pat >= beta)
            return pat;

        if (alpha < pat)
            alpha = pat;

        board.generateMoves<CAPTURE>(&moves);
        if (!moves.GetSize())
        {
            return pat;
        }
    }
    else // if in check, generate evasions
    {
        board.generateMoves<ALL_MOVES>(&moves);
        if (!moves.GetSize()) // if no moves, checkmate
        {
            return -MATE + ply;
        }
        pat = -MATE; // in check
    }

    MoveSorter sorter(board, &moves, bestEntryMove, ply);

    Move bestM = 0;
    BoardState state;

    while (true)
    {
        Move m = sorter.Next();
        if (m.getMove() == 0)
        {
            break;
        }
        // Delta pruning
        if (!board.getNumChecks() && !m.isType<PROMOTION>())
        {
            Piece capturedPiece = board.getSQ(m.to());
            if (capturedPiece == EMPTY) // en-passant
                capturedPiece = PAWN;
            if (pat + pieceScores[getType(capturedPiece)] < alpha - DELTA)
                continue;
        }

        SearchNode child(node);
        Makemove(m, state, ply);
        Score score = -QSearch(ply + 1, -beta, -alpha, &child);
        Undomove(ply);

        if (score > pat)
        {
            pat = score;
            bestM = m;

            if (score >= beta)
            {
                ttable.SetEntry(board.getHash(), mateToTT(score, ply), 0, NodeBound::Lower, m);
                UPDATE_INFO_QBETACUT(info);
                return score;
            }
            if (score > alpha)
            {
                alpha = score;
            }
        }
    }
    NodeBound bound = (pat >= beta) ? NodeBound::Lower : (pat > originalAlpha) ? NodeBound::Exact : NodeBound::Upper;
    ttable.SetEntry(board.getHash(), mateToTT(pat, ply), 0, bound, bestM);

    return pat;
}

template <NodeType nodeT>
Score Searcher::Search(int depth, int ply, Score alpha, Score beta, SearchNode* node, const bool nullMoveAllowed)
{
    constexpr bool isPVNode = nodeT == PVNode || nodeT == RootNode;
    constexpr bool isRootNode = nodeT == RootNode;

    if (!isRunning.load(std::memory_order::memory_order_relaxed))
        return 0;

    // Sample the clock every 2048 nodes rather than at every node: getTime() was previously
    // called once per node, which is pure overhead on the hottest path in the engine.
    if ((info.numNodes & 2047ULL) == 0 && getTime() - info.startTime > hardLimit)
    {
        isRunning = false;
        return 0;
    }

    if (info.numNodes + info.numQNodes > constraints.maxNodes)
    {
        isRunning = false;
        return 0;
    }

    if (!isRootNode && board.getState()->repetition >= 3)
        return 0; // draw by repetition

    // Note: the 50-move draw is tested after move generation below, because a mate delivered on
    // the 100th ply takes precedence over the draw and that needs the legal move count.

    // See PLY_LIMIT: nothing else bounds `ply`, and exceeding it is an out-of-bounds write.
    if (ply >= PLY_LIMIT)
        return board.getNumChecks() ? 0 : Eval<FULL>(board, accumulators);

    if (depth <= 0)
    {
        return QSearch(ply, alpha, beta, node);
    }

    UPDATE_INFO_NODES(info);
    if (isPVNode && info.seldepth < ply)
    {
        info.seldepth = ply;
    }

    TranspositionEntry* entry = nullptr;
    Score ttOrStaticScore = 0; // set below: from TT score if available, otherwise from staticEval

    Move bestEntryMove = 0;

    if (isRootNode && info.bestmove.move.getMove() != 0)
    {
        bestEntryMove = info.bestmove.move; // use previous search's best move
    }
    else
    {
        entry = ttable.GetEntry(board.getHash());

        if (entry)
        {
            ttOrStaticScore = ttToMate(entry->score, ply);
            entry->setAge(ttable.GetAge()); // reset the age for this node

            UPDATE_INFO_TTHIT(info);
            if constexpr (!isPVNode)
            {
                if (entry->depth >= depth)
                {
                    if ((entry->getNodeBound() == NodeBound::Exact ||
                         (entry->getNodeBound() == NodeBound::Upper && ttOrStaticScore <= alpha) ||
                         (entry->getNodeBound() == NodeBound::Lower && ttOrStaticScore >= beta)))
                    {
                        UPDATE_INFO_TTCUT(info);
                        return ttOrStaticScore;
                    }

                    if (entry->getNodeBound() == NodeBound::Lower)
                        alpha = std::max(alpha, ttOrStaticScore);
                    else if (entry->getNodeBound() == NodeBound::Upper)
                        beta = std::min(beta, ttOrStaticScore);

                    if (alpha >= beta)
                    {
                        UPDATE_INFO_TTCUT(info);
                        return ttOrStaticScore;
                    }
                }
            }

            bestEntryMove = entry->move;
        }
        else if constexpr (!isPVNode)
        {

            // Internal Iterative Reduction if no hashmove found (reduce depth by one)
            depth -= depth > IIR_DEPTH;
        }
    }

    Score staticEval;
    bool inCheck = board.getNumChecks() > 0;

    if (!inCheck)
        staticEval = Eval<FULL>(board, accumulators);
    else if (node->prev && node->prev->prev)
        staticEval = node->prev->prev->staticEval;
    else
        staticEval = 0;

    node->staticEval = staticEval;

    if (!entry)
        ttOrStaticScore = node->staticEval;

    // 50-move draw. Tested here rather than at node entry because checkmate on the 100th ply
    // beats the draw, and that is only knowable once we have a legal move count (handled by the
    // mate/stalemate block above, which runs first). `>=` rather than `==`: the counter is only
    // sampled at nodes we visit, so an exact comparison can be stepped over entirely.
    if (!isRootNode && board.getState()->move50rule >= 100)
        return 0;

    // reverse futility pruning
    if (!isPVNode && !inCheck && depth <= REVERSE_FUTILITY_MAX_DEPTH)
    {
        Score margin = REVERSE_FUTILITY_MULTI * depth;

        if (staticEval - margin >= beta)
        {
            return staticEval;
        }
    }

    // razoring
    if (!isPVNode && !inCheck && depth <= RAZORING_DEPTH)
    {
        Score margin = RAZORING_OFFSET + (RAZORING_MULTI * depth);

        if (staticEval + margin <= alpha)
        {
            return QSearch(ply, alpha, beta, node);
        }
    }

    BoardState state;

    // null move pruning

    int numOurPieces = popCount(board.getBB(ALL_PIECES, board.sideToMove) & ~board.getBB(PAWN));
    if (!isPVNode && numOurPieces > 0 && !inCheck && depth >= NULL_DEPTH && !isLoss(beta) && nullMoveAllowed &&
        staticEval >= beta)
    {
        int newDepth = depth * NULL_MOVE_DEPTH_MULTI - NULL_MOVE_DEPTH_OFFSET;

        SearchNode nullNode(node);
        MakeNullmove(state, ply);
        Score nullScore = -Search<CUTNode>(newDepth, ply + 1, -beta, -beta + 1, &nullNode);
        UndoNullmove(ply);
        if (nullScore >= beta)
        {
            // Verification search. The null move was already undone above, so this re-searches
            // the *current* position and must use `ply`, not `ply + 1` -- mate scores round-trip
            // through mateToTT/ttToMate keyed on ply, so an off-by-one here corrupts them.
            SearchNode verifyNode(node);
            Score score = Search<CUTNode>(newDepth, ply, beta - 1, beta, &verifyNode, false);
            if (score >= beta)
                return nullScore;
        }
    }

    MoveList moves;
    board.generateMoves<ALL_MOVES>(&moves);

    if (moves.GetSize() == 0)
    {
        Score mateScore = 0; // stalemate
        if (inCheck)         // if in check, then checkmate
            mateScore = -MATE + ply;

        ttable.SetEntry(board.getHash(), mateToTT(mateScore, ply), depth, NodeBound::Exact, 0);
        return mateScore;
    }

    MoveSorter sorter(board, &moves, bestEntryMove, ply);

    Score bestS = -INF;
    Move bestM = 0;

    NodeBound nodeBound = NodeBound::Upper;
    Move firstMove = 0;
    int lmpCount = 0;
    const int lmpThreshold = LMP_OFFSET + LMP_MULTI * depth * depth;

    for (int i = 0;; i++)
    {
        Move move = sorter.Next();
        if (move.getMove() == 0)
        {
            break; // end of moves
        }

        if (!isPVNode && !inCheck && move.isType<QUIET>())
        {
            if (lmpCount++ >= lmpThreshold) // lmp
                continue;
        }
        int extension = 0;
        bool checkMove = board.isCheckMove(move);

        if (checkMove)
            extension = 1;
        // if (isPVNode && move.to() == board.getState()->move.to()) // recapture extension
        //     extension = 1;

        // Futility pruning
        if (!isPVNode && !inCheck && depth < FUTILITY_DEPTH && move.type() == QUIET && !checkMove)
        {
            Score eval = ttOrStaticScore + FUTILITY_MARGIN(depth);
            if (eval <= alpha)
                continue;
        }

        SearchNode child(node);
        Makemove(move, state, ply);
        ttable.Prefetch(board.getHash());

        Score score;
        bool fullSearch = i == 0; // always full search the first move
        if (fullSearch)
            firstMove = move;

        if (!fullSearch) // pvs
        {
            UPDATE_INFO_PVSZEROWINDOW(info);

            // Late move reductions (LMR)
            int reductions = 0;
            if (depth >= LMR_DEPTH && i >= LMR_INDEX && !inCheck && !checkMove && move.isType<QUIET>()) // lmr
                reductions = LMRReduction(depth, i);

            score = -Search<CUTNode>(std::max(depth - reductions - 1, 0), ply + 1, -alpha - 1, -alpha, &child);

            if (score > alpha)
            {
                UPDATE_INFO_PVSFAILHIGH(info);
            }
            else
            {
                UPDATE_INFO_PVSFAILLOW(info);
            }

            fullSearch = score > alpha && (isPVNode || reductions != 0 || extension > 0);
            if (fullSearch)
            {
                UPDATE_INFO_PVSRESEARCH(info);
            }
            if (reductions > 0)
            {
                UPDATE_INFO_LMRREDUCE(info);
                UPDATE_INFO_LMRREDUCT(info, reductions);

                if (score > alpha)
                {
                    UPDATE_INFO_LMRFAILHIGH(info);
                }
                else
                {
                    UPDATE_INFO_LMRFAILLOW(info);
                }

                if (fullSearch)
                {
                    UPDATE_INFO_LMRRESEARCH(info);
                }
            }
        }

        if (fullSearch && isRunning.load(std::memory_order::memory_order_relaxed))
        {
            if constexpr (isPVNode)
                score = -Search<PVNode>(depth + extension - 1, ply + 1, -beta, -alpha, &child);
            else
                score = -Search<CUTNode>(depth + extension - 1, ply + 1, -alpha - 1, -alpha, &child);
        }
        Undomove(ply);

        // update root move score if we are root node
        if constexpr (isRootNode)
        {
            for (int i = 0; i < info.rootMoves.numRoots; i++)
            {
                if (info.rootMoves[i].move == move)
                {
                    info.rootMoves[i].score = score;
                    break;
                }
            }
        }

        if (!isRunning.load(std::memory_order::memory_order_relaxed))
            return 0;

        if (score >= beta)
        {
            if (move.isType<CAPTURE>())
            {
                addCaptureBonus(CapturedType(board, move), move, depth); // add move history bonus
            }
            else
            {
                if (board.getState()->move.getMove() != 0) // don't add for null moves
                    counterMove[board.getState()->move.from()][board.getState()->move.to()] = move;

                addKillerMove(ply, move);
                addHistoryBonus(board.whiteToMove, move, depth); // add move history bonus
                updateContinuationHistory(board, move, depth, false);
            }

            for (unsigned int p = moves.GetSize() - i; p < moves.GetSize(); p++)
            {
                Move penaltyMove = sorter.moves[p].m;
                if (penaltyMove == move)
                    continue;

                if (penaltyMove.isType<QUIET>())
                {
                    addHistoryPenalty(board.whiteToMove, penaltyMove, depth);
                    updateContinuationHistory(board, penaltyMove, depth, true);
                }
                else if (penaltyMove.isType<CAPTURE>())
                {
                    addCapturePenalty(CapturedType(board, penaltyMove), penaltyMove, depth);
                }
            }

            ttable.SetEntry(board.getHash(), mateToTT(score, ply), depth, NodeBound::Lower, move);
            UPDATE_INFO_BETACUT(info);
            UPDATE_INFO_BETACUTMOVE(info, i);
            return score;
        }
        if (score > bestS)
        {
            if (score > alpha)
            {
                nodeBound = NodeBound::Exact;
                alpha = score;

                if constexpr (isPVNode)
                {
                    UpdatePV(&node->pvLine, move, &child.pvLine);
                }
            }
            bestS = score;
            bestM = move;

            if constexpr (isRootNode)
            {
                info.pv = node->pvLine;
                info.bestmove = {bestM, bestS};

                if (!constraints.quiet)
                {
                    uint64_t time = getTime() - info.startTime;
                    uint64_t nodes = info.numNodes + info.numQNodes;
                    uint64_t nps = nodes * 1000 / std::max(time, 1ull);

                    std::cout << "info depth " << depth << " currmove " << bestM.toString() << " " << ScoreToUCI(score)
                              << " time " << time << " nodes " << nodes << " nps " << nps << " pv "
                              << GetMoveListString(&info.pv) << std::endl;
                }
            }
        }
    }

    if (firstMove == bestM)
    {
        UPDATE_INFO_PVHIT(info);
    }

    if (moves.GetSize() >= 2)
    {
        UPDATE_INFO_ORDERHIT(info);
    }
    // Every move was pruned (futility/LMP). staticEval is unrelated to the window and can sit
    // above alpha, which would report a bound we never established; alpha is the correct
    // fail-low value here.
    if (bestM.getMove() == 0)
        return alpha;

    if (isRunning.load(std::memory_order::memory_order_relaxed)) // don't store in transposition table if we cutoff
                                                                 // early (Time cutoff, node cutoff, etc.)
        ttable.SetEntry(board.getHash(), mateToTT(bestS, ply), depth, nodeBound, bestM);

    return bestS;
}

void Searcher::IterativeDeepening(Board& board)
{
    SearchNode origin(nullptr);

    accumulators.SetCurrent(0);
    auto& originAcc = accumulators.Current();

    board.ResetWhiteAccumulator(originAcc.whiteAcc);
    board.ResetBlackAccumulator(originAcc.blackAcc);
    originAcc.isBlackComputed = originAcc.isWhiteComputed = true;

    RootMove prevBestMove;
    info.bestmove.score = 0;
    prevBestMove.score = 0;
    prevBestMove.move = 0;

    // d < MAX_DEPTH, not <=: lmrTable is indexed [depth][moveNum] with MAX_DEPTH rows, so
    // searching at depth == MAX_DEPTH (reachable via `go depth 256`) reads off the end of it.
    for (unsigned int d = 1; d <= constraints.maxDepth && d < MAX_DEPTH; d++)
    {
        info.seldepth = 0;

        Score delta = ASPIRATION_STARTING_DELTA;
        Score alpha = prevBestMove.score - delta;
        Score beta = prevBestMove.score + delta;

        if (d == 1)
        {
            alpha = -INF;
            beta = INF;
        }

        while (true)
        {
            if (!isRunning.load(std::memory_order::memory_order_relaxed))
                break;

            SearchNode rootNode(&origin);
            Score eval = Search<RootNode>(d, 0, alpha, beta, &rootNode);

            delta *= ASPIRATION_MULTIPLIER;
            if (eval > alpha && eval < beta)
                break;
            else if (eval <= alpha)
            {
                alpha = std::max(eval - delta, -INF);
            }
            else
            {
                beta = std::min(eval + delta, INF);
            }
        }

        if (!constraints.quiet)
        {
            uint64_t time = getTime() - info.startTime;
            uint64_t nodes = info.numNodes + info.numQNodes;
            uint64_t nps = nodes * 1000 / std::max(time, 1ull);

            std::cout << "info depth " << d << " seldepth " << info.seldepth + 1 << " currmove "
                      << info.bestmove.move.toString() << " " << ScoreToUCI(info.bestmove.score) << " nodes " << nodes
                      << " time " << time << " nps " << nps << " hashfull " << (int)(ttable.GetFull() * 1000) << " pv "
                      << GetMoveListString(&info.pv) << std::endl;
        }

        if (!isRunning.load(std::memory_order::memory_order_relaxed))
        {
            info.bestmove = prevBestMove;
            break;
        }

        prevBestMove = info.bestmove;

        // Soft limit: this iteration completed, but there is not enough time left to make a
        // meaningful start on the next one. Stopping here rather than at the hard limit is what
        // keeps the engine from burning its whole budget on an iteration it cannot finish.
        if (getTime() - info.startTime >= softLimit)
            break;
    }
}

void Searcher::ComputeMovetime()
{
    constexpr unsigned long long NO_LIMIT = ~0ULL;

    // `go infinite` -- run until an explicit `stop`.
    if (constraints.infinite)
    {
        softLimit = hardLimit = NO_LIMIT;
        return;
    }

    // `go movetime N` -- spend exactly N, no soft cutoff.
    if (constraints.movetime > 0)
    {
        softLimit = hardLimit = constraints.movetime;
        return;
    }

    // No clock information at all (e.g. `go depth N` / `go nodes N` / bench).
    if (constraints.remainingTime == 0)
    {
        softLimit = hardLimit = NO_LIMIT;
        return;
    }

    // Reserve a small buffer so we never flag on the way back through the GUI.
    constexpr unsigned long long OVERHEAD = 20;
    const unsigned long long remaining =
        constraints.remainingTime > OVERHEAD ? constraints.remainingTime - OVERHEAD : 1;
    const unsigned long long inc = constraints.increment;

    unsigned long long soft;
    if (constraints.movesToGo > 0)
    {
        // Fixed number of moves until the next control: divide what's left, plus most of the
        // increment we are guaranteed to get back.
        soft = remaining / std::max(constraints.movesToGo, 1u) + (inc * 3) / 4;
    }
    else
    {
        // Sudden death or increment-only. The increment term is the important part: at a control
        // like 8+0.08 the base share alone is a small fraction of what we can actually afford,
        // and ignoring the increment (as this previously did) throws away most of the budget.
        soft = remaining / 20 + (inc * 3) / 4;
    }

    // Widen or narrow slightly by how many root moves there are -- a near-forced position does
    // not need the full share.
    if (info.rootMoves.numRoots > 30)
        soft = soft * 3 / 2;
    else if (info.rootMoves.numRoots < 10)
        soft = soft * 3 / 4;

    // The hard limit is what an in-progress iteration may not exceed. Never commit more than a
    // fraction of the clock to a single move regardless of what the increment suggests.
    unsigned long long hard = std::min(remaining * 2 / 5, soft * 5);

    soft = std::min(soft, hard);

    // The soft limit gates *starting* another iteration, and each iteration costs roughly 1.5-2x
    // the previous one. Comparing raw elapsed against the full budget therefore overshoots badly:
    // an iteration begun at 99% of budget still runs to completion. Starting only while under
    // ~3/5 of the budget keeps the typical move close to its share.
    soft = soft * 3 / 5;

    softLimit = std::max(soft, 1ULL);
    hardLimit = std::max(hard, 1ULL);
}

void Searcher::DoSearch()
{
    info = {};
    std::memset(killerMoves, 0, sizeof(killerMoves));

    info.startTime = getTime();

    info.rootMoves.Clear();
    info.bestmove = RootMove{0, 0};

    constraints.maxDepth = constraints.maxDepth == 0 ? MAX_DEPTH : constraints.maxDepth;
    constraints.maxNodes = constraints.maxNodes == 0 ? UINT_MAX : constraints.maxNodes;
    ComputeMovetime();

    MoveList mlist;
    board.generateMoves<ALL_MOVES>(&mlist);

    for (Move* i = mlist.moves; i < mlist.end; i++)
    {
        info.rootMoves.Add(RootMove{*i, 0});
    }

    ttable.IncrementAge();
    IterativeDeepening(board);

    if (!constraints.quiet)
    {
        std::cout << "bestmove " << info.bestmove.move.toString() << std::endl;

#ifdef SEARCHINFO
        PrintDebugInfo(info);
#endif
    }

    Stop();
}

void Searcher::WorkerLoop()
{
    while (true)
    {
        {
            std::unique_lock lock(mtx);
            cv.wait(lock, [&] { return isRunning == true || isQuit == true; });
        }

        if (isQuit)
        {
            break;
        }

        DoSearch();

        {
            std::lock_guard lock(mtx);
            isSearching = false;
        }
        cv.notify_all();
    }
}

void Searcher::StartSearch(const Board& board, const SearchConstraints& constraints)
{
    Stop();

    std::unique_lock lock(mtx);
    cv.wait(lock, [this] { return !isSearching; });

    this->board = board;
    this->constraints = constraints;
    isRunning = true;
    isSearching = true;

    cv.notify_all();
}

void Searcher::Stop()
{
    isRunning = false;
}

void Searcher::WaitForSearch()
{
    std::unique_lock lock(mtx);
    cv.wait(lock, [this] { return !isSearching; });
}

void Searcher::ResizeTT(unsigned long long bytes)
{
    // Never reallocate under a live search -- the worker holds raw entry pointers.
    Stop();
    WaitForSearch();

    unsigned long long targetBuckets = bytes / sizeof(TranspositionBucket);
    if (targetBuckets == 0)
        targetBuckets = 1;

    // Round down to a power of two: GetEntry/SetEntry index with `key & (numBuckets - 1)`, which
    // only distributes correctly when numBuckets is a power of two.
    unsigned long long pow2 = 1;
    while (pow2 <= targetBuckets / 2)
        pow2 *= 2;

    ttable.Resize(pow2 * sizeof(TranspositionBucket));
}
