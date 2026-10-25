#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstring>
#include <iostream>

#include "search.h"

#include "MoveSort.h"
#include "SearchNode.h"
#include "evaluate.h"
#include "profile.h"
#include "time.h"
#include "transposition.h"
#include "types.h"

#ifdef TUNING
#include "tuning_params.h"
#else
#include "searchParams.h"
#endif

#define INF 32000
#define MATE 31000

#define FUTILITY_MARGIN(DEPTH) (FUTILITY_OFFSET + FUTILITY_MULTI * (DEPTH))

// Hard ceiling on search ply. `ply` is NOT bounded by `depth`: a check extension keeps depth
// constant while ply still increments, so a forcing sequence can climb indefinitely. The
// accumulator array holds exactly MAX_DEPTH nodes and Makemove() writes index ply+1, so without
// this cap a long check sequence writes past the end of that allocation. Leave headroom for the
// qsearch that runs on top of the deepest main-search ply.
constexpr int PLY_LIMIT = MAX_DEPTH - 8;

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

    if (((info.numNodes + info.numQNodes) & 2047ULL) == 0 && getTime() - info.startTime > hardLimit)
    {
        isRunning = false;
        return 0;
    }

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
    Score rawEval = INVALID_SCORE;

    // Generate moves

    MoveList moves;
    if (!board.getNumChecks()) // If not in check, generate captures
    {
        if (entry && entry->staticEval != INVALID_SCORE)
            rawEval = entry->staticEval;
        else
            rawEval = Eval<FULL>(board, accumulators);

        pat = rawEval;

        if (pat >= beta)
        {
            if (!entry)
                ttable.SetEntry(board.getHash(), rawEval, pat, 0, NodeBound::Lower, 0);
            return pat;
        }

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
        // don't search bad captures if not in check
        if (!board.getNumChecks() && sorter.currentBucketNum == MoveSorter::BAD_CAPTURE)
            break;
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
                ttable.SetEntry(board.getHash(), rawEval, mateToTT(score, ply), 0, NodeBound::Lower, m);
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
    ttable.SetEntry(board.getHash(), rawEval, mateToTT(pat, ply), 0, bound, bestM);

    return pat;
}

template <NodeType nodeT>
Score Searcher::Search(int depth, int ply, Score alpha, Score beta, SearchNode* node, const bool nullMoveAllowed)
{
    constexpr bool isPVNode = nodeT == PVNode || nodeT == RootNode;
    constexpr bool isRootNode = nodeT == RootNode;
    constexpr bool isCutNode = nodeT == CutNode;

    // step 1. check for stopping conditions
    if (!isRunning.load(std::memory_order::memory_order_relaxed))
        return 0;

    if (((info.numNodes + info.numQNodes) & 2047ULL) == 0 && getTime() - info.startTime > hardLimit)
    {
        isRunning = false;
        return 0;
    }

    if (info.numNodes + info.numQNodes > constraints.maxNodes)
    {
        isRunning = false;
        return 0;
    }

    // clear pv line on pv nodes
    if constexpr (isPVNode)
        pvlines[ply].len = 0;

    // don't exceed the max ply on search
    if (ply >= PLY_LIMIT)
        return board.getNumChecks() ? 0 : Eval<FULL>(board, accumulators);

    // step 2. qsearch
    if (depth <= 0)
    {
        return QSearch(ply, alpha, beta, node);
    }

    // step 3. check for draws
    if (!isRootNode && board.getState()->repetition >= 3)
        return 0; // draw by 3-fold repetition
    if (!isRootNode && board.getState()->move50rule >= 100)
        return 0;

    UPDATE_INFO_NODES(info);
    if (isPVNode && info.seldepth < ply)
    {
        info.seldepth = ply;
    }

    // step 4. probe transposition table (unless root node)
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
        if constexpr (!isPVNode)
        {

            // Internal Iterative Reduction if no hashmove found (reduce depth by one)
            depth -= depth > IIR_DEPTH && bestEntryMove.getMove() == 0;
        }
    }

    // step 5. Statically evaluate the board using the nnue or from the tt entry
    Score& staticEval = node->staticEval;
    bool inCheck = board.getNumChecks() > 0;

    if (inCheck)
    {
        if (node->prev && node->prev->prev && node->prev->prev->staticEval != INVALID_SCORE)
            staticEval = node->prev->prev->staticEval;
        else
            staticEval = INVALID_SCORE;
    }
    else if (entry && entry->staticEval != INVALID_SCORE)
        staticEval = entry->staticEval;
    else
        staticEval = Eval<FULL>(board, accumulators);

    if (!entry)
        ttOrStaticScore = node->staticEval;

    // step 6. reverse futility pruning
    if (!isPVNode && !inCheck && depth <= REVERSE_FUTILITY_MAX_DEPTH)
    {
        Score margin = REVERSE_FUTILITY_MULTI * depth;

        if (staticEval - margin >= beta)
        {
            return staticEval;
        }
    }

    // step 7. razoring
    if (!isPVNode && !inCheck && depth <= RAZORING_DEPTH)
    {
        Score margin = RAZORING_OFFSET + (RAZORING_MULTI * depth);

        if (staticEval + margin <= alpha)
        {
            return QSearch(ply, alpha, beta, node);
        }
    }

    // step 8. null move pruning

    BoardState state;

    int numOurPieces = popCount(board.getBB(board.sideToMove, ALL_PIECES) & ~board.getBB(PAWN));
    if (!isPVNode && numOurPieces > 0 && !inCheck && depth >= NULL_DEPTH && !isLoss(beta) && nullMoveAllowed &&
        staticEval >= beta)
    {
        int newDepth = depth * NULL_MOVE_DEPTH_MULTI - NULL_MOVE_DEPTH_OFFSET;

        SearchNode nullNode(node);
        MakeNullmove(state, ply);
        Score nullScore = -Search<CutNode>(newDepth, ply + 1, -beta, -beta + 1, &nullNode);
        UndoNullmove(ply);
        if (nullScore >= beta)
        {
            if (isWin(nullScore))
                nullScore = beta;

            // skip verification search if depth is below a certain amount
            if (depth < NULL_MOVE_VERIFY_DEPTH)
            {
                ttable.SetEntry(board.getHash(), node->staticEval, mateToTT(nullScore, ply), depth, NodeBound::Lower,
                                0);
                return nullScore;
            }

            // verify null search
            SearchNode verifyNode(node);
            Score score = Search<CutNode>(newDepth, ply, beta - 1, beta, &verifyNode, false);
            if (score >= beta)
                return nullScore;
        }
    }

    // step 9. generate moves
    MoveList moves;
    board.generateMoves<ALL_MOVES>(&moves);

    // step 10. check for stalemate and checkmate
    if (moves.GetSize() == 0)
    {
        Score mateScore = 0; // stalemate
        if (inCheck)         // if in check, then checkmate
            mateScore = -MATE + ply;

        ttable.SetEntry(board.getHash(), node->staticEval, mateToTT(mateScore, ply), depth, NodeBound::Exact, 0);
        return mateScore;
    }

    // step 11. iterate through moves
    Move quietsTried[64], capturesTried[32];
    int numQuietsTried = 0, numCapturesTried = 0;

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

        if constexpr (isRootNode)
        {
            // don't search previously checked pv lines for multiPV
            unsigned int i = 0;
            bool searched = false;
            while (i < constraints.multiPV && info.multiPVLines[i].len)
            {
                if (info.multiPVLines[i++].moves[0] == move)
                {
                    searched = true;
                    break;
                }
            }
            if (searched)
                continue;
        }

        // step 12. late move pruning
        if (!isPVNode && !inCheck && move.isType<QUIET>())
        {
            if (lmpCount++ >= lmpThreshold) // lmp
                continue;
        }
        int extension = 0;
        bool checkMove = board.isCheckMove(move);

        // step 13. check extensions
        if (checkMove)
            extension = 1;
        // if (isPVNode && move.to() == board.getState()->move.to()) // recapture extension
        //     extension = 1;

        // step 14. Futility pruning
        if (!isPVNode && !inCheck && depth < FUTILITY_DEPTH && move.type() == QUIET && !checkMove)
        {
            Score eval = ttOrStaticScore + FUTILITY_MARGIN(depth);
            if (eval <= alpha)
                continue;
        }

        bool fullSearch = i == 0; // always full search the first move
        if (fullSearch)
            firstMove = move;

        // set up child node
        Score score;
        SearchNode child(node);

        // make the move
        Makemove(move, state, ply);
        ttable.Prefetch(board.getHash());

        // step 15. zero window search on any move other than first
        if (!fullSearch) // pvs
        {
            UPDATE_INFO_PVSZEROWINDOW(info);

            // step 16. Late move reductions (LMR)
            int reductions = 0;
            if (depth >= LMR_DEPTH && i >= LMR_INDEX && !inCheck && !checkMove)
            {
                if (move.isType<QUIET>())
                {
                    reductions = LMRReduction(depth, i) + isCutNode;
                }
                else if (sorter.currentBucketNum == MoveSorter::BAD_CAPTURE)
                    reductions = std::max(LMRReduction(depth, i) - 1, 0);

                // reductions = std::max(reductions - isPVNode, 0);
            }

            score =
                -Search<CutNode>(std::max(depth - reductions + extension - 1, 0), ply + 1, -alpha - 1, -alpha, &child);

            if (score > alpha)
            {
                UPDATE_INFO_PVSFAILHIGH(info);
            }
            else
            {
                UPDATE_INFO_PVSFAILLOW(info);
            }

            fullSearch = score > alpha && reductions != 0; // re-search if score exceeds alpha and reductions were made
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

        // step 16. full searches
        // do a full search cutnode search if alpha was exceeded
        if (fullSearch && !(isPVNode && i == 0))
        {
            score = -Search<CutNode>(depth + extension - 1, ply + 1, -alpha - 1, -alpha, &child);
        }

        // on pv nodes, search as a pv node if a full search cut node exceeded alpha
        if (isPVNode && (i == 0 || score > alpha))
        {
            score = -Search<PVNode>(depth + extension - 1, ply + 1, -beta, -alpha, &child);
        }
        Undomove(ply);

        if (!isRunning.load(std::memory_order::memory_order_relaxed))
            return 0;

        // step 17. update root move score if we are root node
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

        // step 18. beta cutoff if score met beta
        if (score >= beta)
        {
            // update move heuristics on beta cutoff

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

            for (int j = 0; j < numQuietsTried; j++)
            {
                Move penaltyMove = quietsTried[j];
                addHistoryPenalty(board.whiteToMove, penaltyMove, depth);
                updateContinuationHistory(board, penaltyMove, depth, true);
            }

            for (int j = 0; j < numCapturesTried; j++)
            {
                Move penaltyMove = capturesTried[j];
                addCapturePenalty(CapturedType(board, penaltyMove), penaltyMove, depth);
            }

            ttable.SetEntry(board.getHash(), node->staticEval, mateToTT(score, ply), depth, NodeBound::Lower, move);
            UPDATE_INFO_BETACUT(info);
            UPDATE_INFO_BETACUTMOVE(info, i);
            return score;
        }

        // step 19. update new best move
        if (score > bestS)
        {
            if (score > alpha)
            {
                nodeBound = NodeBound::Exact;
                alpha = score;

                if constexpr (isPVNode)
                {
                    UpdatePV(&pvlines[ply], move, &pvlines[ply + 1]);
                }
            }
            bestS = score;
            bestM = move;
        }

        // step 20. add move to tried move arrays for heuristics in case of beta cutoff
        if (move.isType<CAPTURE>() &&
            numCapturesTried < static_cast<int>(sizeof(capturesTried) / sizeof(capturesTried[0])))
        {
            capturesTried[numCapturesTried++] = move;
        }
        else if (move.isType<QUIET>() &&
                 numQuietsTried < static_cast<int>(sizeof(quietsTried) / sizeof(quietsTried[0])))
        {
            quietsTried[numQuietsTried++] = move;
        }
    }

    if (firstMove == bestM)
        UPDATE_INFO_PVHIT(info); // first move was the best move

    if (moves.GetSize() >= 2)
        UPDATE_INFO_ORDERHIT(info); // node had more than 1 move, so move ordering was done

    // Every move was pruned (futility/LMP), so node failed low
    if (bestM.getMove() == 0)
        return alpha;

    ttable.SetEntry(board.getHash(), node->staticEval, mateToTT(bestS, ply), depth, nodeBound, bestM);
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

    Score linescores[MAX_DEPTH] = {};

    for (unsigned int d = 1; d <= constraints.maxDepth && d < MAX_DEPTH; d++)
    {
        for (unsigned int i = 0; i < constraints.multiPV; i++)
            info.multiPVLines[i].len = 0;

        for (unsigned int i = 0; i < constraints.multiPV; i++)
        {
            info.seldepth = 0;
            Score eval;

            Score delta = ASPIRATION_STARTING_DELTA;
            Score alpha = linescores[i] - delta;
            Score beta = linescores[i] + delta;

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
                eval = Search<RootNode>(d, 0, alpha, beta, &rootNode);

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

            if (!isRunning.load(std::memory_order::memory_order_relaxed))
            {
                break;
            }

            info.multiPVLines[i] = pvlines[0];
            linescores[i] = eval;

            if (i == 0)
                info.bestmove = {info.multiPVLines[0].moves[0], eval};

            if (!constraints.quiet)
            {
                uint64_t time = getTime() - info.startTime;
                uint64_t nodes = info.numNodes + info.numQNodes;
                uint64_t nps = nodes * 1000 / std::max(time, static_cast<uint64_t>(1));

                std::cout << "info depth " << d;
                if (constraints.multiPV > 1)
                {
                    std::cout << " multipv " << i + 1;
                }
                std::cout << " seldepth " << info.seldepth + 1 << " " << ScoreToUCI(eval) << " nodes " << nodes
                          << " time " << time << " nps " << nps << " hashfull " << (int)(ttable.GetFull() * 1000)
                          << " pv " << GetMoveListString(&info.multiPVLines[i]) << std::endl;
            }
            if (!isRunning.load(std::memory_order::memory_order_relaxed))
            {
                break;
            }
        }

        if (!isRunning.load(std::memory_order::memory_order_relaxed))
        {
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
    const unsigned long long remaining =
        constraints.remainingTime > constraints.moveOverhead ? constraints.remainingTime - constraints.moveOverhead : 1;
    const unsigned long long inc = constraints.increment;

    unsigned long long soft;
    if (constraints.movesToGo > 0)
    {
        soft = remaining / std::max(constraints.movesToGo, 1u) + (inc * 3) / 4;
    }
    else
    {
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

    MoveList mlist;
    board.generateMoves<ALL_MOVES>(&mlist);

    for (Move* i = mlist.moves; i < mlist.end; i++)
    {
        info.rootMoves.Add(RootMove{*i, 0});
    }

    if (info.rootMoves.numRoots == 0) // check if we're in mate or stalemate
    {
        int score = 0;
        if (board.getNumChecks() > 0)
            score = -MATE;

        std::cout << "info depth 0 " << ScoreToUCI(score) << "\n";
        std::cout << "bestmove (none)" << std::endl;
    }
    else
    {

        constraints.multiPV = std::clamp(constraints.multiPV, 1u, (unsigned int)info.rootMoves.numRoots);

        ComputeMovetime();

        ttable.IncrementAge();
        IterativeDeepening(board);

        if (!constraints.quiet)
        {
            // A search stopped before finishing depth 1 has no best move; never send a null bestmove.
            if (info.bestmove.move.getMove() == 0 && info.rootMoves.numRoots > 0)
                info.bestmove.move = info.rootMoves[0].move;

            std::cout << "bestmove " << info.bestmove.move.toString() << std::endl;

#ifdef SEARCHINFO
            PrintDebugInfo(info);
#endif
        }
    }

    Stop();
}

void Searcher::WorkerLoop()
{
    while (true)
    {
        {
            std::unique_lock lock(mtx);
            // Wait on isSearching, not isRunning: a Stop() landing between StartSearch() and this thread
            // waking clears isRunning, and the worker would sleep forever while WaitForSearch() waits.
            cv.wait(lock, [&] { return isSearching == true || isQuit == true; });
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
