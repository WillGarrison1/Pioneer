#include "direction.h"
#include "square.h"

Direction directionsTable[64][64];

void initDirection()
{
    for (Square first = SQ_A1; first <= SQ_H8; first++)
    {
        for (Square second = SQ_A1; second <= SQ_H8; second++)
        {
            File firstFile = getFile(first);
            Rank firstRank = getRank(first);

            File secondFile = getFile(second);
            Rank secondRank = getRank(second);

            if (first == second)
                directionsTable[first][second] = NONE_DIR;
            else if (firstRank == secondRank)
            {
                if (firstFile > secondFile)
                    directionsTable[first][second] = WEST;
                else
                    directionsTable[first][second] = EAST;
            }
            else if (firstFile == secondFile)
            {
                if (firstRank > secondRank)
                    directionsTable[first][second] = SOUTH;
                else
                    directionsTable[first][second] = NORTH;
            }
            else if (firstFile - secondFile == firstRank - secondRank)
            {
                if (firstRank > secondRank)
                    directionsTable[first][second] = SOUTH_WEST;
                else
                    directionsTable[first][second] = NORTH_EAST;
            }
            else if (firstFile - secondFile == secondRank - firstRank)
            {
                if (firstRank > secondRank)
                    directionsTable[first][second] = SOUTH_EAST;
                else
                    directionsTable[first][second] = NORTH_WEST;
            }
            else
            {
                directionsTable[first][second] = NONE_DIR;
            }
        }
    }
}