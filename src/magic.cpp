
#include "magic.h"
#include "bitboard.h"
#include "random.h"
#include "square.h"
#include "time.h"
#include <fstream>
#include <iostream>
#include <windows.h>

Bitboard _rookBlockers[64][4096];
Bitboard _bishopBlockers[64][512];

Bitboard noEdgeMask[64];

Bitboard *slidingMoves;

// Rook magic (800kb)
Magic rookMagics[64];

// Bishop magics (41kb)
Magic bishopMagics[64];

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

void InitNoEdgeMasks()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard mask = 0;

        File file = getFile(s);
        Rank rank = getRank(s);

        mask |= sqrToBB(s);

        if (file)
            mask |= fileBBs[FILE_A];
        if (file != FILE_H)
            mask |= fileBBs[FILE_H];
        if (rank)
            mask |= rankBBs[RANK_1];
        if (rank != RANK_8)
            mask |= rankBBs[RANK_8];

        noEdgeMask[s] = ~mask;
    }
}

void GenerateRookMoves(Bitboard *rookMoves, unsigned long long size)
{
    Bitboard *pointer = rookMoves;
    memset(rookMoves, 0, size * 8.0f);
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = rookMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic &magic = rookMagics[s];

        magic.moves = pointer;
        magic.mask = blockerMask;

        pointer += numBlockers;

        for (unsigned int i = 0; i < numBlockers; i++)
        {
            Bitboard blocker = getBlockers(blockerMask, i);

            int index = _pext_u64(blocker, blockerMask);

            magic.moves[index] = sendRay(s, NORTH, blocker) | sendRay(s, SOUTH, blocker) | sendRay(s, EAST, blocker) |
                                 sendRay(s, WEST, blocker);
        }
    }
}

void GenerateBishopMoves(Bitboard *bishopMoves, unsigned long long size)
{
    Bitboard *pointer = bishopMoves;
    memset(bishopMoves, 0, size * 8.0f);

    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = bishopMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic &magic = bishopMagics[s];

        magic.moves = pointer;
        magic.mask = blockerMask;

        pointer += numBlockers;

        for (unsigned int i = 0; i < numBlockers; i++)
        {
            Bitboard blocker = getBlockers(blockerMask, i);

            int index = _pext_u64(blocker, blockerMask);

            magic.moves[index] = sendRay(s, NORTH_EAST, blocker) | sendRay(s, SOUTH_WEST, blocker) |
                                 sendRay(s, SOUTH_EAST, blocker) | sendRay(s, NORTH_WEST, blocker);
        }
    }
}

void InitMagics()
{
    InitNoEdgeMasks();

    unsigned long long bishopSize = 0;

    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = bishopMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        bishopSize += numBlockers;
    }

    std::cout << "Bishop magic size: " << (unsigned long long)((float)bishopSize / 1024.0f * 8.0f) << "kb" << std::endl;

    unsigned long long rookSize = 0;

    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = rookMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        rookSize += numBlockers;
    }

    std::cout << "Rook magic size: " << (unsigned long long)((float)rookSize / 1024.0f * 8.0f) << "kb" << std::endl;

    slidingMoves = new Bitboard[rookSize + bishopSize];

    GenerateBishopMoves(slidingMoves + rookSize, bishopSize);
    GenerateRookMoves(slidingMoves, rookSize);
}

#ifdef MAGIC_GEN

unsigned long long getMagicSize(Magic *magics)
{
    unsigned long long size = 0;
    for (int i = 0; i < 64; i++)
    {
        size += (1 << magics[i].offset) * 8;
    }
    return size;
}

void GenerateRookBlockers()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = rookMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        for (unsigned long long i = 0; i < numBlockers; i++)
        {
            Bitboard blockers = getBlockers(blockerMask, i);
            _rookBlockers[s][i] = blockers;
        }
    }
}

void GenerateBishopBlockers()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = bishopMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        for (unsigned int i = 0; i < numBlockers; i++)
        {
            Bitboard blocker = getBlockers(blockerMask, i);
            _bishopBlockers[s][i] = blocker;
        }
    }
}

bool TestMagic(Key magicNum, Bitboard *list, unsigned int size, unsigned short numBits)
{
    bool *test = new bool[1 << numBits];

    memset(test, 0, 1 << numBits);

    bool success = true;

    for (unsigned int i = 0; i < size; i++)
    {
        Bitboard blockers = list[i];
        int index = (blockers * magicNum) >> (64 - numBits);
        if (test[index])
        {
            success = false;
            break;
        }
        test[index] = true;
    }

    delete[] test;
    return success;
}

void moveCursorUpOneLine()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        COORD pos = csbi.dwCursorPosition;
        if (pos.Y > 0)
            pos.Y -= 1;
        pos.X = 0;
        SetConsoleCursorPosition(hConsole, pos);
    }
}

void GenerateMagics()
{
    for (int i = 0; i < 64; i++)
    {

        rookMagics[i].magic = 0ULL;
        rookMagics[i].offset = popCount(rookMasks[i] & noEdgeMask[i]) + 4;

        bishopMagics[i].magic = 0ULL;
        bishopMagics[i].offset = popCount(bishopMasks[i] & noEdgeMask[i]) + 4;
    }

    while (true)
    {
        for (Square s = SQ_A1; s <= SQ_H8; s++)
        {
            unsigned long long magicNum = RandNum() & RandNum() & RandNum();

            unsigned int rookSize = 1 << popCount(rookMasks[s] & noEdgeMask[s]);
            unsigned int bishopSize = 1 << popCount(bishopMasks[s] & noEdgeMask[s]);

            unsigned char rookOffset = rookMagics[s].offset - 1;
            unsigned char bishopOffset = bishopMagics[s].offset - 1;

            while (TestMagic(magicNum, _rookBlockers[s], rookSize, rookOffset))
            {
                rookMagics[s].magic = magicNum;
                rookMagics[s].offset = rookOffset;
                rookOffset--;

                moveCursorUpOneLine();
                std::cout << "Rook Size: " << (unsigned int)(getMagicSize(rookMagics) / 1024) << " kb            "
                          << "\nBishop Size: " << (unsigned int)(getMagicSize(bishopMagics) / 1024) << " kb           ";
            }

            magicNum = RandNum() & RandNum() & RandNum();

            while (TestMagic(magicNum, _bishopBlockers[s], bishopSize, bishopOffset))
            {
                bishopMagics[s].magic = magicNum;
                bishopMagics[s].offset = bishopOffset;
                bishopOffset--;
                std::cout << "\rBishop Size: " << (unsigned int)(getMagicSize(bishopMagics) / 1024) << " kb           ";
            }
        }
    }
}

void OutputMagics()
{
    std::fstream magicOut("magicOut.txt", std::ios::out);
    magicOut << "Magic rookMagics[] = {\n";
    for (int i = 0; i < 64; i++)
    {
        magicOut << "Magic(" << rookMagics[i].magic << "ULL, " << (unsigned int)rookMagics[i].offset << "),\n";
    }
    magicOut << "};\n";
    magicOut << "Magic bishopMagics[] = {\n";
    for (int i = 0; i < 64; i++)
    {
        magicOut << "Magic(" << bishopMagics[i].magic << "ULL, " << (unsigned int)bishopMagics[i].offset << "),\n";
    }
    magicOut << "};" << std::endl;

    magicOut.close();
}

BOOL WINAPI ExitCatcher(DWORD event)
{
    switch (event)
    {
    case CTRL_C_EVENT:
        std::cout << "\nexiting..." << std::endl;
        OutputMagics();
        exit(0);
    case CTRL_BREAK_EVENT:
        std::cout << "\nexiting..." << std::endl;
        OutputMagics();
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
    InitNoEdgeMasks();

    BENCHMARK();
    GenerateRookBlockers();
    GenerateBishopBlockers();

    GenerateMagics();

    GenerateRookMoves();
    GenerateBishopMoves();
}
#endif