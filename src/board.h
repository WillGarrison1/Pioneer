#ifndef BOARD_H
#define BOARD_H

#include <string>

#include "bitboard.h"
#include "move.h"
#include "piece.h"
#include "types.h"

#include "movegen.h"

#define MAX_PLY 1024

static const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Contains all the information about the current state of the board
struct BoardState
{
    BoardState* prev;

    Bitboard attacks[2]; // the attacks a side has (white = 0 black = 1)
    Bitboard checkers;
    Key zobristHash;
    Piece captured;
    Piece moved;

    Score pawn_material;
    Score non_pawn_material;
    CastlingRights castling;       // Castling rights
    Move move;                     // Move that led to this board state
    unsigned char enPassantSquare; // En passant square
    unsigned char repetition;
    unsigned char move50rule;

    BoardState();
    BoardState(BoardState* prev);

    ~BoardState();
};

class Board
{
  public:
    Board();
    ~Board();

    void addPiece(const Piece piece, const Square square);
    void removePiece(const Square square);
    void movePiece(const Square from, const Square to);

    void addPieceZobrist(const Piece piece, const Square to, Key& key);
    void removePieceZobrist(const Square from, Key& key);
    void movePieceZobrist(const Square from, const Square to, Key& key); // moves the piece and updates the zobrist hash

    void makeMove(const Move move, BoardState* newState);
    void undoMove();

    void makeNullMove(BoardState* state);
    void undoNullMove();

    template <MoveType type>
    inline void generateMoves(MoveList* list)
    {
        if (whiteToMove)
            ::generateMoves<type, WHITE>(*this, list);
        else
            ::generateMoves<type, BLACK>(*this, list);
    }

    /**
     * @brief checks if a move causes check
     *
     * @param move move to check
     * @return bool
     */
    bool isCheckMove(Move move);

    void print() const;

    void setFen(const std::string& fen, BoardState* newState);
    std::string getFen() const;

    // Clears bitboards and board
    void clear();

    void generateAttackBB(const Color side);
    void computeAttackedBBs();

    Bitboard getAttackers(Square sqr);

    void computePins(Bitboard& pinnedS, Bitboard& pinnedD);

    // Get bitboards

    // Get bitboard for piecetypes
    inline Bitboard getBB(const PieceType piece) const
    {
        return pieceBB[piece];
    }

    // Get bitboard for multiple piecetypes (ripped from stockfish)
    template <typename... PieceTypes>
    inline Bitboard getBB(const PieceType piece, const PieceTypes... pieces) const
    {
        return getBB(piece) | getBB(pieces...);
    }

    // Get bitboard for color
    inline Bitboard getBB(const Color color) const
    {
        return colorBB[color];
    }

    // Get bitboard for multiple colors
    template <typename... Colors>
    inline Bitboard getBB(const Color color, const Colors... colors) const
    {
        return (getBB(color) | getBB(colors...));
    }

    template <typename... PieceTypes>
    inline Bitboard getBB(const Color color, const PieceType piece, const PieceTypes... pieces) const
    {
        return getBB(color) & getBB(piece, pieces...);
    }

    // Get piece on square
    inline Piece getSQ(const Square square) const
    {
        return board[square];
    }

    /**
     * @brief Returns the bitboard of attacked squared by the specified color
     *
     * @param c color
     * @return Bitboard
     */
    inline Bitboard getAttacked(const Color c) const
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

    inline const BoardState* getState() const
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

    unsigned int getRepetition() const; // gets the amount of times this position appeared on the board

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
    unsigned short ply;

    Bitboard pieceBB[ALL_PIECES + 1]; // Bitboards for each piece type
    Bitboard colorBB[BLACK + 1];      // Bitboards for each color

    Piece board[64]; // Board representation

    BoardState* state; // Current board state
};
#endif