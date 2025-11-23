#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "direction.h"
#include "evaluate.h"
#include "movegen.h"
#include "piece.h"
#include "profile.h"
#include "square.h"
#include "transposition.h"

BoardState::BoardState()
{
    this->attacks[0] = 0ULL;
    this->attacks[1] = 0ULL;

    move = 0;
    castling = NONE_CASTLE;
    enPassantSquare = SQ_NONE;

    checkers = 0;
    move50rule = 0;

    pawn_material = 0;
    non_pawn_material = 0;

    zobristHash = 0ULL;
}

BoardState::BoardState(BoardState* prev)
{
    this->attacks[0] = 0ULL;
    this->attacks[1] = 0ULL;

    this->move = 0;

    this->castling = prev->castling;
    this->enPassantSquare = SQ_NONE; //* note: only set if en passant can be played
    this->non_pawn_material = prev->non_pawn_material;
    this->pawn_material = prev->pawn_material;
    this->zobristHash = prev->zobristHash;
    this->move50rule = prev->move50rule + 1;
}

BoardState::~BoardState()
{
}

Board::Board()
{
    // Value-initialize each BoardState instead of using memset on non-trivial types.
    state = nullptr;

    whiteToMove = true;
    sideToMove = WHITE;
}

Board::~Board()
{
}

void Board::addPiece(Piece piece, Square square)
{
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    setBit(pieceBB[pieceType], square);
    setBit(colorBB[color], square);

    board[square] = piece;
}

void Board::removePiece(Square square)
{
    const Piece piece = board[square];
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    clearBit(pieceBB[pieceType], square);
    clearBit(colorBB[color], square);

    board[square] = EMPTY;
}

void Board::movePiece(Square from, Square to)
{
    const Piece piece = board[from];
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    clearBit(pieceBB[pieceType], from);
    clearBit(colorBB[color], from);

    setBit(pieceBB[pieceType], to);
    setBit(colorBB[color], to);

    board[from] = EMPTY;
    board[to] = piece;
}

void Board::addPieceZobrist(Piece piece, Square square, Key* key)
{
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    *key ^= boardHashes[square][piece];

    setBit(pieceBB[pieceType], square);
    setBit(colorBB[color], square);

    board[square] = piece;
}

void Board::removePieceZobrist(Square square, Key* key)
{
    const Piece piece = board[square];
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    *key ^= boardHashes[square][piece];

    clearBit(pieceBB[pieceType], square);
    clearBit(colorBB[color], square);

    board[square] = EMPTY;
}

void Board::movePieceZobrist(Square from, Square to, Key* key)
{
    const Piece piece = board[from];
    const PieceType pieceType = getType(piece);
    const Color color = getColor(piece);

    clearBit(pieceBB[pieceType], from);
    clearBit(colorBB[color], from);

    setBit(pieceBB[pieceType], to);
    setBit(colorBB[color], to);

    *key ^= boardHashes[to][piece] ^ boardHashes[from][piece];

    board[from] = EMPTY;
    board[to] = piece;
}

void Board::makeMove(Move move, BoardState* newState)
{
    PROFILE_FUNC();

    // Initialize new board state
    newState->attacks[0] = 0ULL;
    newState->attacks[1] = 0ULL;

    newState->move = move;
    newState->checkers = 0;
    newState->captured = board[move.to()];

    newState->castling = state->castling;
    newState->enPassantSquare = SQ_NONE; //* note: only set if en passant can be played
    newState->non_pawn_material = state->non_pawn_material;
    newState->pawn_material = state->pawn_material;
    newState->zobristHash = state->zobristHash;
    newState->move50rule = state->move50rule + 1;

    if (state->enPassantSquare != SQ_NONE) // if enPassant square from last move, remove it from zobrist
        newState->zobristHash ^= enPassantHash[getFile(getEnPassantSqr())];

    newState->zobristHash ^= castleRightsHash[newState->castling]; // remove old castling rights hash

    // Update board
    if (move.type() == CASTLE)
    {

        Square to = move.to();
        bool kingSide = getFile(to) == FILE_G;

        if (whiteToMove)
        {
            if (kingSide)
            {
                movePieceZobrist(SQ_E1, SQ_G1, &newState->zobristHash);
                movePieceZobrist(SQ_H1, SQ_F1, &newState->zobristHash);
            }
            else
            {
                movePieceZobrist(SQ_E1, SQ_C1, &newState->zobristHash);
                movePieceZobrist(SQ_A1, SQ_D1, &newState->zobristHash);
            }

            newState->castling &= ~(CASTLE_WK | CASTLE_WQ);
        }
        else
        {
            if (kingSide)
            {
                movePieceZobrist(SQ_E8, SQ_G8, &newState->zobristHash);
                movePieceZobrist(SQ_H8, SQ_F8, &newState->zobristHash);
            }
            else
            {
                movePieceZobrist(SQ_E8, SQ_C8, &newState->zobristHash);
                movePieceZobrist(SQ_A8, SQ_D8, &newState->zobristHash);
            }

            newState->castling &= ~(CASTLE_BK | CASTLE_BQ);
        }
    }
    else
    {
        Square from = move.from();
        Square to = move.to();

        Piece piece = board[from];
        PieceType pieceType = getType(piece);
        Color color = getColor(piece);

        Piece captured = board[to];

        // Update castling rights
        if (pieceType == KING && newState->castling)
        {

            if (color == WHITE)
                newState->castling &= ~(CASTLE_WK | CASTLE_WQ);
            else
                newState->castling &= ~(CASTLE_BK | CASTLE_BQ);
        }
        else if (state->castling)
        {
            if (from == SQ_A1 || to == SQ_A1)
                newState->castling &= ~CASTLE_WQ;
            if (from == SQ_H1 || to == SQ_H1)
                newState->castling &= ~CASTLE_WK;
            if (from == SQ_A8 || to == SQ_A8)
                newState->castling &= ~CASTLE_BQ;
            if (from == SQ_H8 || to == SQ_H8)
                newState->castling &= ~CASTLE_BK;
        }

        // Update bitboards

        if (move.to() == getEnPassantSqr() && getType(piece) == PAWN)
        {
            newState->move50rule = 0; // reset 50 move repetition counter on en passant
            Square attacked = to - (whiteToMove ? NORTH : SOUTH);
            removePieceZobrist(attacked, &newState->zobristHash);

            newState->pawn_material -= pieceScores[PAWN] * (whiteToMove ? -1 : 1);
        }
        else if (captured != EMPTY)
        {
            newState->move50rule = 0; // reset 50 move repetition counter on capture
            removePieceZobrist(to, &newState->zobristHash);

            switch (getType(captured))
            {
            case PAWN:
                newState->pawn_material -= pieceScores[PAWN] * (whiteToMove ? -1 : 1);
                break;
            default:
                newState->non_pawn_material -= pieceScores[getType(captured)] * (whiteToMove ? -1 : 1);
                break;
            }
        }

        movePieceZobrist(from, to, &newState->zobristHash);

        if (pieceType == PAWN)
        {
            newState->move50rule = 0; // reset 50 move repetition counter
            if (move.type() == PROMOTION)
            {
                PieceType promote = move.promotion();
                removePieceZobrist(to, &newState->zobristHash);
                addPieceZobrist(makePiece(promote, color), to, &newState->zobristHash);

                newState->pawn_material -= pieceScores[PAWN] * (whiteToMove ? 1 : -1);
                newState->non_pawn_material += pieceScores[promote] * (whiteToMove ? 1 : -1);
            }
            else if (abs(getRank(from) - getRank(to)) == 2) // If pawn jump, and enemy can take, set enPassantSquare
            {
                Bitboard takers = 0ULL;
                File toFile = getFile(to);
                if (toFile != FILE_A)
                {
                    takers |= sqrToBB(to - 1);
                }
                if (toFile != FILE_H)
                {
                    takers |= sqrToBB(to + 1);
                }

                if (getBB(PAWN, ~sideToMove) & takers)
                {
                    newState->enPassantSquare = static_cast<Square>(to - (whiteToMove ? NORTH : SOUTH));
                    newState->zobristHash ^= enPassantHash[getFile(to)];
                }
            }
        }
    }

    // Sync all-piece bb
    pieceBB[ALL_PIECES] = colorBB[WHITE] | colorBB[BLACK];
    pieceBB[EMPTY] = ~pieceBB[ALL_PIECES];

    // Update state
    ply++;

    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;

    newState->zobristHash ^= isBlackHash;
    newState->zobristHash ^= castleRightsHash[newState->castling];
    newState->prev = state;
    state = newState;

    state->repetition = getRepetition();

    computeAttackedBBs();
    state->checkers = getAttackers(lsb(getBB(sideToMove, KING)));
    // assert(getBB(sideToMove, KING) != 0);
}

void Board::undoMove()
{
    PROFILE_FUNC();

    // Get information from state and delete it
    Move move = state->move;

    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;

    if (move.type() == CASTLE)
    {
        Square to = move.to();
        bool kingSide = getFile(to) == FILE_G;

        if (whiteToMove)
        {
            if (kingSide)
            {
                movePiece(SQ_G1, SQ_E1);
                movePiece(SQ_F1, SQ_H1);
            }
            else
            {
                movePiece(SQ_C1, SQ_E1);
                movePiece(SQ_D1, SQ_A1);
            }
        }
        else
        {
            if (kingSide)
            {
                movePiece(SQ_G8, SQ_E8);
                movePiece(SQ_F8, SQ_H8);
            }
            else
            {
                movePiece(SQ_C8, SQ_E8);
                movePiece(SQ_D8, SQ_A8);
            }
        }
    }
    else
    {

        Square from = move.from();
        Square to = move.to();

        Piece piece = board[to];
        Color color = getColor(piece);

        Piece captured = state->captured;

        // Set Piece BBs
        movePiece(to, from);

        if (state->prev && move.to() == state->prev->enPassantSquare)
        {
            Square attacked =
                to - (whiteToMove ? NORTH : SOUTH); // Switched directions because whiteToMove hasn't been reversed yet
            addPiece(makePiece(PAWN, ~sideToMove), attacked);
        }
        else if (captured != EMPTY)
        {
            addPiece(captured, to);
        }

        if (move.type() == PROMOTION)
        {
            removePiece(from);
            addPiece(makePiece(PAWN, color), from);
        }
    }

    // Sync all-piece bb
    pieceBB[ALL_PIECES] = colorBB[WHITE] | colorBB[BLACK];
    pieceBB[EMPTY] = ~pieceBB[ALL_PIECES];

    state = state->prev;
    ply--;
}

void Board::makeNullMove(BoardState* newState)
{
    *newState = *state;
    newState->prev = state;
    state = newState;

    newState->move = 0;
    newState->checkers = 0;

    newState->enPassantSquare = SQ_NONE; //* note: only set if en passant can be played

    if (state->enPassantSquare != SQ_NONE) // if enPassant square from last move, remove it from zobrist
        newState->zobristHash ^= enPassantHash[getFile(getEnPassantSqr())];

    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;

    newState->zobristHash ^= isBlackHash;
}

void Board::undoNullMove()
{
    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;
    state = state->prev;
}

unsigned int Board::getRepetition()
{
    Key zobrist = state->zobristHash;

    BoardState* currState = state->prev;
    while (currState != nullptr)
    {
        if (currState->zobristHash == zobrist)
            return currState->repetition + 1;
        currState = currState->prev;
    }

    return 1;
}

void Board::print() const
{

    Square i = SQ_A8;

    std::cout << "+---+---+---+---+---+---+---+---+\n";
    do
    {
        std::cout << "| " << pieceToString(board[i]) << " ";

        // Have to do funky stuff because Square is unsigned
        if (i % 8 == 7)
        {
            std::cout << "|\n+---+---+---+---+---+---+---+---+\n";
            i -= 15;
        }
        else
        {
            i++;
        }
    } while (i != SQ_H1 - 15);

    std::cout << std::endl;
}

void Board::clear()
{
    // Clear piece BBs
    for (int i = 0; i <= static_cast<int>(ALL_PIECES); i++)
    {
        pieceBB[i] = 0;
    }

    // Fill empty BB
    pieceBB[EMPTY] = -1;

    // Clear color BBs
    colorBB[WHITE] = 0;
    colorBB[BLACK] = 0;

    // Clear board
    for (int i = 0; i < 64; i++)
    {
        board[i] = EMPTY;
    }
}

void Board::setFen(const std::string& fen, BoardState* newState)
{
    clear();

    ply = 0;
    state = newState;
    state->zobristHash = 0ULL;

    std::stringstream ss(fen);
    std::string pos, color, castle, enPassant, noActionRule50;
    ss >> pos;
    Square s = SQ_A8;

    for (char c : pos)
    {
        if (c == ' ')
            break;
        else if (c == '/')
            s -= 16; // Go two ranks down because of the loop increment
        else if (std::isdigit(c))
            s += c - '0';
        else
        {
            Piece piece = stringToPiece(std::string(1, c));
            board[s] = piece;
            setBit(pieceBB[getType(piece)], s);
            setBit(colorBB[getColor(piece)], s);
            state->zobristHash ^= boardHashes[s][piece];

            switch (getType(piece))
            {
            case PAWN:
                state->pawn_material += pieceScores[PAWN] * (getColor(piece) == WHITE ? 1 : -1);
                break;
            default:
                state->non_pawn_material += pieceScores[getType(piece)] * (getColor(piece) == WHITE ? 1 : -1);
                break;
            }

            s++;
        }
    }

    pieceBB[ALL_PIECES] = colorBB[WHITE] | colorBB[BLACK];
    pieceBB[EMPTY] = ~pieceBB[ALL_PIECES];

    ss >> color;
    whiteToMove = color == "w";
    sideToMove = whiteToMove ? WHITE : BLACK;

    if (!whiteToMove)
        state->zobristHash ^= isBlackHash;

    ss >> castle;
    state->castling = NONE_CASTLE;
    for (char c : castle)
    {
        switch (c)
        {
        case 'K':
            state->castling |= CASTLE_WK;
            break;
        case 'Q':
            state->castling |= CASTLE_WQ;
            break;
        case 'k':
            state->castling |= CASTLE_BK;
            break;
        case 'q':
            state->castling |= CASTLE_BQ;
            break;
        }
    }

    state->zobristHash ^= castleRightsHash[state->castling];

    ss >> enPassant;
    if (enPassant != "-")
    {
        File file = File(enPassant[0] - 'a');
        Rank rank = Rank(enPassant[1] - '1');
        state->enPassantSquare = getSquare(file, rank);
        state->zobristHash ^= enPassantHash[file];
    }
    else
    {
        state->enPassantSquare = SQ_NONE;
    }

    ss >> noActionRule50;
    if (noActionRule50 != "-")
        state->move50rule = std::atoi(noActionRule50.c_str());

    computeAttackedBBs();
    state->checkers = getAttackers(lsb(getBB(sideToMove, KING)));
}

// Updates attacked bitboard and returns number of checks of the opposing king
void Board::generateAttackBB(const Color side)
{
    PROFILE_FUNC();
    const Direction forward = side == WHITE ? NORTH : SOUTH;
    const Bitboard enemyKing = getBB(~side, KING);
    const Bitboard blockers = getBB(ALL_PIECES) & ~enemyKing;

    Bitboard pawns = getBB(side, PAWN);
    Bitboard knights = getBB(side, KNIGHT);
    Bitboard bishops = getBB(side, BISHOP);
    Bitboard rooks = getBB(side, ROOK);
    Bitboard queens = getBB(side, QUEEN);

    unsigned int checkCount = 0;
    Bitboard attackBB = 0;

    attackBB |= shift(pawns & ~fileBBs[FILE_A], forward + WEST);
    attackBB |= shift(pawns & ~fileBBs[FILE_H], forward + EAST);

    Square enemyKingSqr = lsb(enemyKing);

    while (knights)
    {
        Square from = popLSB(knights);
        Bitboard moves = knightMoves[from];
        attackBB |= moves;
    }

    while (bishops)
    {
        Square from = popLSB(bishops);
        Bitboard moves = GetBishopMoves(blockers, from);

        attackBB |= moves;
    }

    while (rooks)
    {
        Square from = popLSB(rooks);
        Bitboard moves = GetRookMoves(blockers, from);
        attackBB |= moves;
    }

    while (queens)
    {
        Square from = popLSB(queens);
        Bitboard moves = GetRookMoves(blockers, from) | GetBishopMoves(blockers, from);

        attackBB |= moves;
    }

    Square king = lsb(getBB(side, KING));
    attackBB |= kingMoves[king];

    state->attacks[side == 8] = attackBB;
}

Bitboard Board::getAttackers(Square sqr)
{
    const Bitboard blockers = getBB(ALL_PIECES);
    const Bitboard pawns = getBB(~sideToMove, PAWN);
    const Bitboard knights = getBB(~sideToMove, KNIGHT);
    const Bitboard bishops = getBB(~sideToMove, BISHOP);
    const Bitboard rooks = getBB(~sideToMove, ROOK);
    const Bitboard queens = getBB(~sideToMove, QUEEN);

    Bitboard attackers = 0ULL;
    attackers |= GetBishopMoves(blockers, sqr) & (bishops | queens);
    attackers |= GetRookMoves(blockers, sqr) & (rooks | queens);
    attackers |= knightMoves[sqr] & knights;

    Direction forward = whiteToMove ? NORTH : SOUTH;
    attackers |= (shift(sqrToBB(sqr) & ~fileBBs[FILE_A], forward + WEST) |
                  shift(sqrToBB(sqr) & ~fileBBs[FILE_H], forward + EAST)) &
                 pawns;

    return attackers;
}

// computes the attacked bitboards for boths sides and returns the check bitboard
void Board::computeAttackedBBs()
{
    generateAttackBB(~sideToMove);
    generateAttackBB(sideToMove);
}

void Board::computePins(Bitboard& pinnedS, Bitboard& pinnedD)
{
    pinnedS = pinnedD = 0ULL;

    Square king = lsb(getBB(sideToMove, KING));
    Bitboard friendlyPieces = getBB(sideToMove);
    Bitboard enemyPieces = getBB(~sideToMove);
    Bitboard diagonal = GetBishopMoves(enemyPieces, king);
    Bitboard straight = GetRookMoves(enemyPieces, king);

    Bitboard diagonalAttackers = getBB(~sideToMove, BISHOP, QUEEN) & diagonal;
    Bitboard straightAttackers = getBB(~sideToMove, ROOK, QUEEN) & straight;

    while (diagonalAttackers)
    {
        Square attacker = popLSB(diagonalAttackers);
        if (popCount(bitboardPaths[king][attacker] & friendlyPieces) == 1)
            pinnedD |= bitboardPaths[king][attacker];
    }

    while (straightAttackers)
    {
        Square attacker = popLSB(straightAttackers);
        if (popCount(bitboardPaths[king][attacker] & friendlyPieces) == 1)
            pinnedS |= bitboardPaths[king][attacker];
    }
}

bool Board::isCheckMove(Move move)
{
    const Piece piece =
        (move.type() == PROMOTION) ? makePiece(move.promotion(), getColor(getSQ(move.from()))) : getSQ(move.from());
    const Color side = getColor(piece);
    const PieceType pType = getType(piece);
    const Square to = move.to();

    const Bitboard enemyKing = getBB(~side, KING);
    Bitboard blockers = getBB(ALL_PIECES);

    const Direction dir = directionsTable[to][lsb(enemyKing)];
    // check for direct checks

    switch (pType)
    {
    case PAWN:
        if (pawnAttacks[side][to] & enemyKing)
            return true;
        break;
    case KNIGHT:
        if (knightMoves[to] & enemyKing)
            return true;
        break;
    case BISHOP:
        if ((dir == SOUTH_EAST || dir == SOUTH_WEST || dir == NORTH_WEST ||
             dir == NORTH_EAST) && // if diagonal and no blockers in the way
            !(bitboardPaths[to][lsb(enemyKing)] & blockers & ~enemyKing))
            return true;
        break;
    case ROOK:
        if ((dir == SOUTH || dir == WEST || dir == NORTH || dir == EAST) && // if straight and no blockers in the way
            !(bitboardPaths[to][lsb(enemyKing)] & blockers & ~enemyKing))
            return true;
        break;
    case QUEEN:
        if (dir && !(bitboardPaths[to][lsb(enemyKing)] & blockers &
                     ~enemyKing)) // if there's a direction and no blockers in the way
            return true;
        break;
    case KING:
        if (move.type() == CASTLE) // look for checks from the rook during castling
        {
            const Square rookPos = (move.from() + move.to()) >> 1;
            const Direction rookDir = directionsTable[rookPos][lsb(enemyKing)];

            if (!rookDir) // if there is no direction from where the rook is
                return false;

            Bitboard ray = GetRookMoves(blockers, rookPos);
            if (ray & enemyKing)
                return true;
        }
        break;
    default:
        break;
    }

    // check for revealed checks

    const Direction fromDir = directionsTable[move.from()][lsb(enemyKing)];

    if (!fromDir) // if there is no direction from where the piece was
        return false;

    setBit(blockers, to);
    clearBit(blockers, move.from());

    if (move.to() == getEnPassantSqr() && getType(getSQ(move.from())) == PAWN)
        clearBit(blockers, move.to() - (side == WHITE ? NORTH : SOUTH));

    if (bitboardPaths[move.from()][lsb(enemyKing)] & blockers &
        ~enemyKing) // if there are blockers between the from square and the king
        return false;

    Bitboard ray = (GetBishopMoves(blockers, move.from()) | GetRookMoves(blockers, move.from())) &
                   bitboardRays[-fromDir][move.from()] & getBB(side);
    if (!ray)
        return false;

    PieceType attacker = getType(board[lsb(ray)]);

    switch (attacker)
    {
    case BISHOP:
        return fromDir == SOUTH_EAST || fromDir == SOUTH_WEST || fromDir == NORTH_WEST || fromDir == NORTH_EAST;
    case ROOK:
        return fromDir == EAST || fromDir == WEST || fromDir == NORTH || fromDir == SOUTH;
    case QUEEN:
        return true;
    default:
        return false;
    }
}

std::string Board::getFen() const
{
    std::string fen = "";
    Square s = SQ_A8;
    int emptyC = 0;
    while (s < 64 && s >= 0)
    {
        Piece p = board[s];
        if (p == EMPTY)
            emptyC++;
        else
        {
            if (emptyC)
            {
                fen += std::to_string(emptyC);
                emptyC = 0;
            }
            fen += pieceToString(p);
        }
        s++;
        if (s % 8 == 0)
        {
            if (emptyC)
            {
                fen += std::to_string(emptyC);
                emptyC = 0;
            }
            fen += "/";
            s -= 16;
        }
    }

    fen.resize(fen.size() - 1); // remove trailing /

    fen += whiteToMove ? " w " : " b ";

    if (getState()->castling & CASTLE_WK)
        fen += "K";
    if (getState()->castling & CASTLE_WQ)
        fen += "Q";
    if (getState()->castling & CASTLE_BK)
        fen += "k";
    if (getState()->castling & CASTLE_BQ)
        fen += "q";

    if (!getState()->castling)
        fen += "-";

    if (getState()->enPassantSquare != SQ_NONE)
        fen += " " + sqrToString((Square)getState()->enPassantSquare) + " ";
    else
        fen += " - ";

    fen += std::to_string(getState()->move50rule) + " ";
    fen += std::to_string(int(getState()->move50rule / 2));

    std::cout << fen << std::endl;

    return fen;
}
