#include <cassert>
#include <iostream>
#include <sstream>

#include "bitboard.h"
#include "board.h"
#include "color.h"
#include "direction.h"
#include "evaluate.h"
#include "movegen.h"
#include "piece.h"
#include "square.h"

Board::Board()
{
    // Initialize state
    state = new BoardState();

    state->previous = nullptr;
    state->ply = 0;
    state->move = Move();
    state->castling = NONE_CASTLE;
    state->enPassantSquare = SQ_NONE;
    state->non_pawn_material = 0;
    state->pawn_material = 0;

    whiteToMove = true;
    sideToMove = WHITE;

    setFen(START_FEN);
}

Board::~Board()
{
    // Clean up board states
    while (state != nullptr)
    {
        BoardState* previous = state->previous;
        delete state;
        state = previous;
    }
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

void Board::makeMove(Move move)
{
    // Initialize new board state
    BoardState* newState = new BoardState();

    newState->previous = state;
    newState->move = move;
    newState->ply = state->ply + 1;
    newState->castling = state->castling;
    newState->enPassantSquare = SQ_NONE;
    newState->non_pawn_material = state->non_pawn_material;
    newState->pawn_material = state->pawn_material;

    if (state->previous && state->previous->move.from() == move.to() && state->previous->move.to() == move.from())
        newState->repetitions = state->repetitions + 1;
    else
        newState->repetitions = 0;

    // Update board
    if (move.type() & CASTLE)
    {
        CastlingRights castle = move.castleRights();
        if (whiteToMove)
        {
            if (castle & CASTLE_WK)
            {
                movePiece(SQ_E1, SQ_G1);
                movePiece(SQ_H1, SQ_F1);
            }
            else if (castle & CASTLE_WQ)
            {
                movePiece(SQ_E1, SQ_C1);
                movePiece(SQ_A1, SQ_D1);
            }

            newState->castling &= ~(CASTLE_WK | CASTLE_WQ);
        }
        else
        {
            if (castle & CASTLE_BK)
            {
                movePiece(SQ_E8, SQ_G8);
                movePiece(SQ_H8, SQ_F8);
            }
            else if (castle & CASTLE_BQ)
            {
                movePiece(SQ_E8, SQ_C8);
                movePiece(SQ_A8, SQ_D8);
            }

            newState->castling &= ~(CASTLE_BK | CASTLE_BQ);
        }
    }
    else
    {
        Square from = move.from();
        Square to = move.to();

        Piece piece = move.piece();
        PieceType pieceType = getType(piece);
        Color color = getColor(piece);

        Piece captured = move.captured();

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

        if (move.type() & EN_PASSANT)
        {
            Square attacked = to - (whiteToMove ? NORTH : SOUTH);
            removePiece(attacked);

            newState->pawn_material -= pieceScores[PAWN] * (whiteToMove ? -1 : 1);
        }
        else if (captured != EMPTY)
        {
            removePiece(to);

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

        movePiece(from, to);

        if (move.promotion() != EMPTY)
        {
            PieceType promote = move.promotion();
            removePiece(to);
            addPiece(makePiece(promote, color), to);

            newState->pawn_material -= pieceScores[PAWN] * (whiteToMove ? 1 : -1);
            newState->non_pawn_material += pieceScores[promote] * (whiteToMove ? 1 : -1);
        }

        if (pieceType == PAWN)
        {
            // If pawn jump, set enPassantSquare
            if (abs(getRank(from) - getRank(to)) == 2)
            {
                newState->enPassantSquare = static_cast<Square>(to - (whiteToMove ? NORTH : SOUTH));
            }
        }
    }

    // Sync all-piece bb
    pieceBB[ALL_PIECES] = colorBB[WHITE] | colorBB[BLACK];
    pieceBB[EMPTY] = ~pieceBB[ALL_PIECES];

    // Update state
    state = newState;

    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;

    state->numChecks = generateAttackBB(state->checkBB, ~sideToMove);

    Bitboard other;
    generateAttackBB(other, sideToMove);
}

void Board::undoMove()
{
    BoardState* previous = state->previous;

    // Get information from state and delete it
    Move move = state->move;

    delete state;
    state = previous;

    whiteToMove = !whiteToMove;
    sideToMove = ~sideToMove;

    if (move.type() & CASTLE)
    {
        CastlingRights castle = move.castleRights();
        if (whiteToMove)
        {
            if (castle & CASTLE_WK)
            {
                movePiece(SQ_G1, SQ_E1);
                movePiece(SQ_F1, SQ_H1);
            }
            else if (castle & CASTLE_WQ)
            {
                movePiece(SQ_C1, SQ_E1);
                movePiece(SQ_D1, SQ_A1);
            }
        }
        else
        {
            if (castle & CASTLE_BK)
            {
                movePiece(SQ_G8, SQ_E8);
                movePiece(SQ_F8, SQ_H8);
            }
            else if (castle & CASTLE_BQ)
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

        Piece piece = move.piece();
        PieceType pieceType = getType(piece);
        Color color = getColor(piece);

        Piece captured = move.captured();
        Color capturedColor = getColor(captured);

        // Set Piece BBs
        movePiece(to, from);

        if (move.type() & EN_PASSANT)
        {
            Square attacked =
                to - (whiteToMove ? NORTH : SOUTH); // Switched directions because whiteToMove hasn't been reversed yet
            addPiece(makePiece(PAWN, ~sideToMove), attacked);
        }
        else if (captured != EMPTY)
        {
            addPiece(captured, to);
        }

        if (move.promotion() != EMPTY)
        {
            removePiece(from);
            addPiece(makePiece(PAWN, color), from);
        }
    }

    // Sync all-piece bb
    pieceBB[ALL_PIECES] = colorBB[WHITE] | colorBB[BLACK];
    pieceBB[EMPTY] = ~pieceBB[ALL_PIECES];
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
    for (int i = 0; i <= ALL_PIECES; i++)
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

void Board::setFen(const std::string& fen)
{
    clear();

    state->non_pawn_material = 0;
    state->pawn_material = 0;

    std::stringstream ss(fen);
    std::string pos, color, castle, enPassant;
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

    ss >> enPassant;
    if (enPassant != "-")
    {
        File file = File(enPassant[0] - 'a');
        Rank rank = Rank(enPassant[1] - '1');
        state->enPassantSquare = getSquare(file, rank);
    }

    state->numChecks = generateAttackBB(state->checkBB, ~sideToMove);
    state->repetitions = 0;

    Bitboard other;
    generateAttackBB(other, sideToMove);
}

// Updates attacked bitboard and returns number of checks of the opposing king
unsigned int Board::generateAttackBB(Bitboard& checkBB, const Color side)
{
    const Direction forward = side == WHITE ? NORTH : SOUTH;
    const Bitboard enemyKing = getBB(~side, KING);
    const Bitboard blockers = getBB(ALL_PIECES) & ~enemyKing;

    Bitboard pawns = getBB(side, PAWN);
    Bitboard knights = getBB(side, KNIGHT);
    Bitboard bishops = getBB(side, BISHOP);
    Bitboard rooks = getBB(side, ROOK);
    Bitboard queens = getBB(side, QUEEN);

    unsigned int checkCount = 0;
    state->attacks[side] = 0ULL;

    checkBB = -1ULL;

    state->attacks[side] |= shift(pawns & ~fileBBs[FILE_A], forward + WEST);
    state->attacks[side] |= shift(pawns & ~fileBBs[FILE_H], forward + EAST);

    if (state->attacks[side] & enemyKing)
    {
        // Should be our side to move to reverse attack move
        checkBB = pawnAttacks[~side][lsb(enemyKing)] & pawns;
        checkCount++;
    }

    while (knights)
    {
        Square from = popLSB(knights);
        Bitboard moves = knightMoves[from];
        if (moves & enemyKing)
        { // Knight checks king
            checkBB = sqrToBB(from);
            checkCount++;
        }
        state->attacks[side] |= moves;
    }

    while (bishops)
    {
        Square from = popLSB(bishops);
        Bitboard moves = GetBishopMoves(blockers, from);
        if (moves & enemyKing)
        {
            checkBB = sqrToBB(from) | bitboardPaths[from][lsb(enemyKing)];
            checkCount++;
        }
        state->attacks[side] |= moves;
    }

    while (rooks)
    {
        Square from = popLSB(rooks);
        Bitboard moves = GetRookMoves(blockers, from);
        if (moves & enemyKing)
        {
            checkBB = sqrToBB(from) | bitboardPaths[from][lsb(enemyKing)];
            checkCount++;
        }
        state->attacks[side] |= moves;
    }

    while (queens)
    {
        Square from = popLSB(queens);
        Bitboard moves = GetRookMoves(blockers, from) | GetBishopMoves(blockers, from);
        if (moves & enemyKing)
        {
            checkBB = sqrToBB(from) | bitboardPaths[from][lsb(enemyKing)];
            checkCount++;
        }
        state->attacks[side] |= moves;
    }

    Square king = lsb(getBB(side, KING));
    state->attacks[side] |= kingMoves[king];

    return checkCount;
}

bool Board::isCheckMove(Move move)
{
    const Piece piece = (move.type() & PROMOTION) ? makePiece(move.promotion(), getColor(move.piece())) : move.piece();
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
        if (move.type() & CASTLE) // look for checks from the rook during castling
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

    if (move.type() & EN_PASSANT)
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
    return "";
}
