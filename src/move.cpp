#include "move.h"
#include "piece.h"
#include "square.h"

#include <string>

Move::Move(const std::string str)
{
    Move move;
    File fFile = File(str[0] - 'a');
    Rank fRank = Rank(str[1] - '1');

    File tFile = File(str[2] - 'a');
    Rank tRank = Rank(str[3] - '1');
    PieceType promote = EMPTY;
    if (str.size() == 5)
    {
        promote = static_cast<PieceType>(stringToPiece(std::string(1, str[4])) & 0b111);
    }

    Square from = getSquare(fFile, fRank);
    Square to = getSquare(tFile, tRank);

    m_move = (from) | (to << 6) | (promote << 29);
}

std::string Move::toString()
{
    std::string out;

    Square frm = from();
    File fFile = getFile(frm);
    Rank fRank = getRank(frm);

    Square t = to();
    File tFile = getFile(t);
    Rank tRank = getRank(t);

    out += char('a' + fFile);
    out += char('1' + fRank);
    out += char('a' + tFile);
    out += char('1' + tRank);

    if (promotion() != EMPTY)
    {
        out += pieceToString(makePiece(promotion(), BLACK));
    }

    return out;
}