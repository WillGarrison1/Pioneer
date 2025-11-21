#ifndef BOARD_H
#define BOARD_H

#include <string>

#include "move.h"
#include "piece.h"
#include "types.h"
#include "bitboard.h"

#define MAX_PLY 256

static const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Contains all the information about the current state of the board
struct BoardState
{
    char enPassantSquare;    // En passant square
    CastlingRights castling; // Castling rights
    Move move;               // Move that led to this board state

    Bitboard attacks[2]; // the attacks a side has (white = 0 black = 1)
    Bitboard checkers;

    unsigned char move50rule;
    Score pawn_material;
    Score non_pawn_material;

    BoardState *prev;

    Key zobristHash;

    BoardState();
    BoardState(BoardState *prev);

    ~BoardState();
};

class Board
{
public:
    Board();
    ~Board();

    void addPiece(Piece piece, Square square);
    void removePiece(Square square);
    void movePiece(Square from, Square to);

    void addPieceZobrist(Piece piece, Square to, Key *key);
    void removePieceZobrist(Square from, Key *key);
    void movePieceZobrist(Square from, Square to, Key *key); // moves the piece and updates the zobrist hash

    void makeMove(Move move, BoardState *newState);
    void undoMove();

    void makeNullMove(BoardState *state);
    void undoNullMove();

    /**
     * @brief checks if a move causes check
     *
     * @param move move to check
     * @return bool
     */
    bool isCheckMove(Move move);

    void print() const;

    void setFen(const std::string &fen, BoardState *newState);
    std::string getFen() const;

    // Clears bitboards and board
    void clear();

    void generateAttackBB(const Color side);
    void computeAttackedBBs();

    Bitboard getAttackers(Square sqr);

    void computePins(Bitboard &pinnedS, Bitboard &pinnedD);

    // Move stuff
    constexpr Move createMove(Square from, Square to, MoveType mType = QUIET, PieceType promote = EMPTY,
                              CastlingRights castle = NONE_CASTLE) const
    {
        return Move(from, to, board[from], board[to], mType, castle, promote);
    }

    // Get bitboards

    // Get bitboard for piecetypes
    inline Bitboard getBB(PieceType piece) const
    {
        return pieceBB[piece];
    }

    // Get bitboard for multiple piecetypes (ripped from stockfish)
    template <typename... PieceTypes>
    inline Bitboard getBB(PieceType piece, PieceTypes... pieces) const
    {
        return getBB(piece) | getBB(pieces...);
    }

    // Get bitboard for color
    inline Bitboard getBB(Color color) const
    {
        return colorBB[color];
    }

    // Get bitboard for multiple colors
    template <typename... Colors>
    inline Bitboard getBB(Color color, Colors... colors) const
    {
        return (getBB(color) | getBB(colors...));
    }

    template <typename... PieceTypes>
    inline Bitboard getBB(Color color, PieceType piece, PieceTypes... pieces) const
    {
        return getBB(color) & getBB(piece, pieces...);
    }

    // Get piece on square
    inline Piece getSQ(Square square) const
    {
        return board[square];
    }

    /**
     * @brief Returns the bitboard of attacked squared by the specified color
     *
     * @param c color
     * @return Bitboard
     */
    inline Bitboard getAttacked(Color c) const
    {
        return state->attacks[c == BLACK];
    }

    /**
     * @brief Get the number of checks on the king
     *
     * @return unsigned int
     */
    inline unsigned int getNumChecks() const
    {
        return popCount(state->checkers);
    }

    inline Bitboard getCheckers() const
    {
        return state->checkers;
    }

    inline Square getEnPassantSqr() const
    {
        return static_cast<Square>(state->enPassantSquare);
    }

    inline const BoardState *getState() const
    {
        return state;
    }

    inline Score getPawnMaterial() const
    {
        return state->pawn_material;
    }

    inline Score getNonPawnMaterial() const
    {
        return state->non_pawn_material;
    }

    unsigned int getRepetition(); // gets the amount of times this position appeared on the board

    inline Key getHash() const
    {
        return state->zobristHash;
    }

    inline unsigned int getPly() const
    {
        return ply;
    }

    bool whiteToMove; // True if white is to move
    Color sideToMove; // Side to move

private:
    unsigned char ply;

    Bitboard pieceBB[ALL_PIECES + 1]; // Bitboards for each piece type
    Bitboard colorBB[BLACK + 1];      // Bitboards for each color

    Piece board[64]; // Board representation

    BoardState *state; // Current board state
};

constexpr static int size = sizeof(Board);

#endif