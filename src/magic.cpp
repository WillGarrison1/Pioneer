
#include "magic.h"
#include "bitboard.h"
#include "square.h"
#include "time.h"
#include <cstdlib>
#include <iostream>
#include <windows.h>

Bitboard _rookMoves[64][16384];
Bitboard* _bishopMoves[64];

#ifdef MAGIC_GEN

unsigned long long RandNum()
{
    return ((unsigned long long)rand() << 48) | ((unsigned long long)rand() << 32) |
           ((unsigned long long)rand() << 16) | ((unsigned long long)rand());
}

Bitboard getBlockers(Bitboard mask, unsigned int number)
{
    Bitboard out = 0;
    Bitboard temp = mask;
    Square s;

    while (temp && number)
    {
        s = popLSB(temp);
        out |= (Bitboard)(number & 1) << s;

        number >>= 1;
    }

    return out;
}

void GenerateRookMoves()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        File file = getFile(s);
        Rank rank = getRank(s);
        Bitboard blockerMask =
            bitboardRays[EAST][s] | bitboardRays[WEST][s] | bitboardRays[NORTH][s] | bitboardRays[SOUTH][s];

        blockerMask &= ~sqrToBB(s); // remove origin from the possible blockers

        for (unsigned long long i = 0; i < 16384; i++) // always 16383 (2^14-1) possible blockers
        {
            Bitboard blockers = getBlockers(blockerMask, i);

            Bitboard moves = sendRay(s, EAST, blockers) | sendRay(s, WEST, blockers) | sendRay(s, NORTH, blockers) |
                             sendRay(s, SOUTH, blockers);
            _rookMoves[s][i] = moves;
        }
    }
}

void GenerateBishopMoves()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        File file = getFile(s);
        Rank rank = getRank(s);
        Bitboard blockerMask = bitboardRays[NORTH_EAST][s] | bitboardRays[NORTH_WEST][s] | bitboardRays[SOUTH_EAST][s] |
                               bitboardRays[SOUTH_WEST][s];

        blockerMask &= ~sqrToBB(s); // remove origin from the possible blockers

        // number of blockers for bishops is this formula: 1 << (7+distanceFromEdge*2)
        unsigned long long numBlockers = 1 << popCount(blockerMask);

        _bishopMoves[s] = new Bitboard[numBlockers];

        for (unsigned long long i = 0; i < numBlockers; i++)
        {
            Bitboard blockers = getBlockers(blockerMask, i);

            Bitboard moves = sendRay(s, NORTH_EAST, blockers) | sendRay(s, NORTH_WEST, blockers) |
                             sendRay(s, SOUTH_EAST, blockers) | sendRay(s, SOUTH_WEST, blockers);
            _bishopMoves[s][i] = moves;
        }
    }
}

void GenerateMagics()
{
    while (true)
    {
        for (Square s = SQ_A1; s <= SQ_H8; s++)
        {
            unsigned long long d = RandNum();
            
        }
    }
}

BOOL WINAPI ExitCatcher(DWORD event)
{
    switch (event)
    {
    case CTRL_C_EVENT:
        std::cout << "\nexiting..." << std::endl;
        exit(0);
    case CTRL_BREAK_EVENT:
        std::cout << "\nexiting..." << std::endl;
        exit(0);
    default:
        return FALSE;
    }
}

int main()
{
    if (SetConsoleCtrlHandler(ExitCatcher, TRUE))
    {
        std::cout << "successfully set exit handler!" << std::endl;
    }
    else
    {
        std::cout << "failed to set exit handler" << std::endl;
        return 1;
    }

    initSquare();
    initBBs();

    BENCHMARK();
    srand(time(0));
    GenerateRookMoves();
    GenerateBishopMoves();

    GenerateMagics();
}
#endif