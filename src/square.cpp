#include "square.h"
#include "direction.h"

int _distToEdge[19][64];
const int (*distToEdge)[64] = _distToEdge + 9;

void initSquare()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        File file = getFile(s);
        Rank rank = getRank(s);

        // Straights
        _distToEdge[NORTH + 9][s] = 7 - rank;
        _distToEdge[SOUTH + 9][s] = rank;
        _distToEdge[EAST + 9][s] = 7 - file;
        _distToEdge[WEST + 9][s] = file;

        // Diagonal
        _distToEdge[NORTH_EAST + 9][s] = std::min(7 - rank, 7 - file);
        _distToEdge[NORTH_WEST + 9][s] = std::min(7 - rank, (int)file);
        _distToEdge[SOUTH_EAST + 9][s] = std::min((int)rank, 7 - file);
        _distToEdge[SOUTH_WEST + 9][s] = std::min((int)rank, (int)file);
    }
}