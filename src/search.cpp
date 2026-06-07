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

#define INF 32000
#define MATE 31000

#define FUTILITY_DEPTH 4

constexpr int lmr_index = 2; // the first index lmr will be used on
constexpr int lmr_depth = 2; // the minimum depth lmr can be used

constexpr Score aspirationStartingDelta = 30;
constexpr float aspirationMultiplier = 1.5f;

#define NULL_DEPTH 3
#define IIR_DEPTH 3 // internal iterative reduction depth
#define FUTILITY_MARGIN(DEPTH) (80 + 120 * (DEPTH))
#define DELTA 200

constexpr auto lmrTable = [] {
    std::array<std::array<int, 256>, MAX_DEPTH> table{};
    for (int d = 0; d < MAX_DEPTH; d++)
    {
        for (int m = 0; m < 256; m++)
        {
            table[d][m] = 0.75f + std::log(d > 0 ? d : 1) * std::log(m > 0 ? m : 1) / 2.25f;
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

    // 50 move and 3 fold draws are checked before QSearch is called

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
        board.generateMoves<CAPTURE>(&moves);
        pat = Eval<FULL>(board, accumulators);
        if (!moves.GetSize())
        {
            return pat;
        }

        if (pat >= beta)
            return pat;

        if (alpha < pat)
            alpha = pat;
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

    MoveSorter sorter(board, &moves, bestEntryMove);

    Move bestM = 0;
    BoardState state;

    while (sorter.size)
    {
        Move m = sorter.Next();

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

    if (getTime() - info.startTime > constraints.movetime)
    {
        isRunning = false;
        return 0;
    }

    if (info.numNodes + info.numQNodes > constraints.maxNodes)
    {
        isRunning = false;
        return 0;
    }

    if (!isRootNode && board.getState()->repetition == 3)
        return 0; // draw by repetition

    if (!isRootNode && board.getState()->move50rule == 100)
        return 0; // 50 fullmoves have been made

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

    // reverse futility pruning
    if (!isPVNode && !inCheck && depth <= 8)
    {
        Score margin = 120 * depth;

        if (staticEval - margin >= beta)
        {
            return staticEval;
        }
    }

    // razoring
    if (!isPVNode && !inCheck && depth <= 3)
    {
        Score margin = 300 + (100 * depth);

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
        int newDepth = depth * 2 / 3 - 1;

        SearchNode nullNode(node);
        MakeNullmove(state, ply);
        Score nullScore = -Search<CUTNode>(newDepth, ply + 1, -beta, -beta + 1, &nullNode);
        UndoNullmove(ply);
        if (nullScore >= beta)
        {
            // Verification search
            SearchNode verifyNode(node);
            Score score = Search<CUTNode>(newDepth, ply + 1, beta - 1, beta, &verifyNode, false);
            if (score >= beta)
                return nullScore;
            else if (bestEntryMove == 0 && verifyNode.pvLine.len > 0)
            {
                bestEntryMove = verifyNode.pvLine.moves[0];
            }
        }
    }

    MoveSorter sorter(board, &moves, bestEntryMove);

    Score bestS = -INF;
    Move bestM = 0;

    NodeBound nodeBound = NodeBound::Upper;
    Move firstMove = 0;
    int lmpCount = 0;
    const int lmpThreshold = 3 + 2 * depth * depth;
    for (int i = 0; sorter.size != 0; i++)
    {
        Move move = sorter.Next();

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
            if (depth >= lmr_depth && i >= lmr_index && !inCheck && !checkMove && move.isType<QUIET>()) // lmr
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
                UPDATE_INFO_PVSRESEARCH(info);

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
                PieceType victimType = getType(board.getSQ(move.to()));
                if (move.to() == board.getEnPassantSqr())
                    victimType = PAWN;
                addCaptureBonus(victimType, move, depth); // add move history bonus
            }
            else
            {
                if (board.getState()->move.getMove() != 0) // don't add for null moves
                    counterMove[board.getState()->move.from()][board.getState()->move.to()] = move;

                addKillerMove(board.getPly(), move);
                addHistoryBonus(board.whiteToMove, move, depth); // add move history bonus
                updateContinuationHistory(board, move, depth, false);
            }

            for (unsigned int p = moves.GetSize() - i; p < moves.GetSize(); p++)
            {
                Move penaltyMove = sorter.moveVals[p].m;
                if (penaltyMove == move)
                    continue;

                if (penaltyMove.isType<QUIET>())
                {
                    addHistoryPenalty(board.whiteToMove, penaltyMove, depth);
                    updateContinuationHistory(board, penaltyMove, depth, true);
                }
                else if (penaltyMove.isType<CAPTURE>())
                {
                    PieceType victimType = getType(board.getSQ(penaltyMove.to()));
                    if (penaltyMove.to() == board.getEnPassantSqr())
                        victimType = PAWN;
                    addCapturePenalty(victimType, penaltyMove, depth);
                }
            }

            ttable.SetEntry(board.getHash(), mateToTT(score, ply), depth, NodeBound::Lower, move);
            UPDATE_INFO_BETACUT(info);
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

                uint64_t time = getTime() - info.startTime;
                uint64_t nodes = info.numNodes + info.numQNodes;
                uint64_t nps = nodes * 1000 / std::max(time, 1ull);

                std::cout << "info depth " << depth << " best " << bestM.toString() << " score cp " << score << " time "
                          << time << " nodes " << info.numNodes + info.numQNodes << " nps " << nps << " pv "
                          << GetMoveListString(&info.pv) << "\n";
            }
        }
    }

    if (firstMove == bestM)
        UPDATE_INFO_PVHIT(info);

    if (moves.GetSize() >= 2)
        UPDATE_INFO_ORDERHIT(info);

    if (bestM.getMove() == 0) // if we didn't search a move (futility pruned all moves)
        return staticEval;    // return static evaluation

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

    for (unsigned int d = 1; d <= constraints.maxDepth; d++)
    {
        info.seldepth = 0;

        Score delta = aspirationStartingDelta;
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

            delta *= aspirationMultiplier;
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

        uint64_t time = getTime() - info.startTime;
        uint64_t nodes = info.numNodes + info.numQNodes;
        uint64_t nps = nodes * 1000 / std::max(time, 1ull);

        std::cout << "info depth " << d << " seldepth " << info.seldepth + 1 << " currmov "
                  << info.bestmove.move.toString() << " score cp " << info.bestmove.score << " nodes " << nodes
                  << " time " << time << " nps " << nps << " hashfull " << (int)(ttable.GetFull() * 1000) << " pv "
                  << GetMoveListString(&info.pv) << "\n";

        if (!isRunning.load(std::memory_order::memory_order_relaxed))
        {
            info.bestmove = prevBestMove;
            break;
        }

        prevBestMove = info.bestmove;
    }
}

void Searcher::ComputeMovetime()
{
    if (constraints.remainingTime > 0)
    {
        float maxTime = static_cast<float>(constraints.remainingTime) / 25.0f;

        // adjust movetime based on how many roots moves exist
        if (info.rootMoves.numRoots > 30)
        {
            maxTime *= 1.5;
        }
        else if (info.rootMoves.numRoots < 10)
        {
            maxTime *= 0.75;
        }
        else
        {
            maxTime *= 0.9;
        }
        constraints.movetime = (unsigned int)std::min(maxTime, (float)constraints.remainingTime *
                                                                   0.2f); // use at most 20% of remaining time
        if (constraints.movetime == 0)
            constraints.movetime = 1;
    }
    else
    {
        constraints.movetime = constraints.movetime == 0 ? UINT_MAX : constraints.movetime;
    }
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

    std::cout << "bestmove " << info.bestmove.move.toString() << std::endl;
    PrintDebugInfo(info);
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