#include "piece.h"

#include <string>

std::string pieceToString(Piece piece)
{
    std::string s = "";
    switch (getType(piece))
    {
    case PAWN:
        s = "P";
        break;
    case KNIGHT:
        s = "N";
        break;
    case BISHOP:
        s = "B";
        break;
    case ROOK:
        s = "R";
        break;
    case QUEEN:
        s = "Q";
        break;
    case KING:
        s = "K";
        break;
    default:
        s = " ";
    }

    if (getColor(piece) == BLACK)
        s[0] = std::tolower(s[0]);

    return s;
}

Piece stringToPiece(const std::string& s)
{
    std::string str = s;
    Color color = std::isupper(str[0]) ? WHITE : BLACK;

    switch (std::toupper(str[0]))
    {
    case 'P':
        return makePiece(PAWN, color);
    case 'N':
        return makePiece(KNIGHT, color);
    case 'B':
        return makePiece(BISHOP, color);
    case 'R':
        return makePiece(ROOK, color);
    case 'Q':
        return makePiece(QUEEN, color);
    case 'K':
        return makePiece(KING, color);
    default:
        return EMPTY;
    }
}