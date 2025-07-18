
#include "magic.h"
#include "bitboard.h"
#include "square.h"
#include "time.h"
#include <fstream>
#include <iostream>
#include <random>
#include <windows.h>

Bitboard _rookBlockers[64][4096];
Bitboard _bishopBlockers[64][512];

Bitboard noEdgeMask[64];

Magic rookMagics[] = {
    Magic(12790223850121986918ULL, 12), Magic(12249781098368212474ULL, 12), Magic(5897472635849906216ULL, 12),
    Magic(6196936030906441591ULL, 12),  Magic(17698275161581183764ULL, 12), Magic(15793201778769842961ULL, 12),
    Magic(8306339439476134652ULL, 12),  Magic(6151919977193487627ULL, 13),  Magic(5165206597265866753ULL, 11),
    Magic(18277366514258780166ULL, 10), Magic(6347260864466620928ULL, 10),  Magic(14805302368746930337ULL, 10),
    Magic(16017709216858094160ULL, 11), Magic(13044113426291894784ULL, 10), Magic(4142186268443807782ULL, 10),
    Magic(16476982223314143764ULL, 11), Magic(6330837869439926276ULL, 11),  Magic(3056257497296806150ULL, 10),
    Magic(6558703408486859141ULL, 10),  Magic(11700741059461075136ULL, 10), Magic(9095920325459809039ULL, 11),
    Magic(4467844809828216831ULL, 11),  Magic(9844002371983638824ULL, 10),  Magic(477476118539285127ULL, 11),
    Magic(12952362722422784422ULL, 11), Magic(18117212201120563968ULL, 10), Magic(8830997577181810720ULL, 10),
    Magic(12910712107420396032ULL, 10), Magic(2002688283135049840ULL, 10),  Magic(14390690389453975612ULL, 10),
    Magic(3342439739838564432ULL, 10),  Magic(7948430193541736028ULL, 11),  Magic(10782954550093416320ULL, 11),
    Magic(7928029706656432481ULL, 10),  Magic(6918233471567544833ULL, 10),  Magic(7009572485926490112ULL, 10),
    Magic(13609401556216286587ULL, 11), Magic(14752103547088701488ULL, 10), Magic(6546207519134468144ULL, 10),
    Magic(14701737702357736645ULL, 11), Magic(5867838507433230337ULL, 11),  Magic(9903421398658236416ULL, 10),
    Magic(4499660238340882448ULL, 10),  Magic(5716209272073879577ULL, 11),  Magic(16271804979515158649ULL, 11),
    Magic(18097152498855444520ULL, 10), Magic(9904816427307368632ULL, 10),  Magic(10964246403367960593ULL, 11),
    Magic(2007200258576174592ULL, 11),  Magic(11545828723695649280ULL, 10), Magic(16118722842297631488ULL, 10),
    Magic(3679913686536820224ULL, 10),  Magic(13883293631214413312ULL, 10), Magic(16152723513087122944ULL, 10),
    Magic(12529138073813357568ULL, 10), Magic(13925239226075386368ULL, 11), Magic(11747360357413409410ULL, 12),
    Magic(16809371501072646210ULL, 11), Magic(11615628794885332995ULL, 11), Magic(4838537262312783897ULL, 11),
    Magic(2570993348352415754ULL, 11),  Magic(9611244568525148198ULL, 11),  Magic(1397426513889399332ULL, 11),
    Magic(15820720534205211974ULL, 12),
};
Magic bishopMagics[] = {
    Magic(2037887731401572866ULL, 6),   Magic(5217524948532412576ULL, 5),  Magic(2130238928551458754ULL, 5),
    Magic(8541105044434268140ULL, 5),   Magic(16393401988395981324ULL, 5), Magic(3522356350843958190ULL, 5),
    Magic(3947506246382302932ULL, 5),   Magic(12349147229492331928ULL, 6), Magic(16290341164032450818ULL, 5),
    Magic(14650861030356327984ULL, 5),  Magic(3351012402451333956ULL, 5),  Magic(1305204024269725174ULL, 5),
    Magic(11548027733246582345ULL, 5),  Magic(5618541832206614870ULL, 5),  Magic(6221385353133825091ULL, 5),
    Magic(10288027952214204446ULL, 5),  Magic(11322091004670847506ULL, 5), Magic(10187601504005983985ULL, 5),
    Magic(6742464305305981184ULL, 7),   Magic(7825576631201824958ULL, 7),  Magic(6791991239691617768ULL, 7),
    Magic(868632357975731226ULL, 7),    Magic(5883675717988254434ULL, 5),  Magic(1886880335138390472ULL, 5),
    Magic(5989841708373810466ULL, 5),   Magic(14849863331485603840ULL, 5), Magic(18269984401444217856ULL, 7),
    Magic(12752786854657679340ULL, 10), Magic(12260913648733593605ULL, 9), Magic(1322670710581202057ULL, 7),
    Magic(11004627062182019632ULL, 5),  Magic(3500143563430272032ULL, 5),  Magic(3527466814576304245ULL, 5),
    Magic(16552127349959164296ULL, 5),  Magic(16715374793201287298ULL, 7), Magic(17798113583656599680ULL, 9),
    Magic(2184039128277205248ULL, 9),   Magic(9818876373509685519ULL, 7),  Magic(13794008893975956494ULL, 5),
    Magic(7434913576083131890ULL, 5),   Magic(8662009847104061675ULL, 5),  Magic(16373636925338451026ULL, 5),
    Magic(15184449781566318607ULL, 7),  Magic(10909054341960727814ULL, 7), Magic(10669968849585075472ULL, 7),
    Magic(55049006488889856ULL, 7),     Magic(4830436217996936065ULL, 5),  Magic(16888190748280094353ULL, 5),
    Magic(4247184797029368115ULL, 5),   Magic(6416719239132495249ULL, 5),  Magic(11030315261464680408ULL, 5),
    Magic(9176632956226637432ULL, 5),   Magic(7063586988708201616ULL, 5),  Magic(18423592912441213639ULL, 5),
    Magic(14141507923977374400ULL, 5),  Magic(7762393944460046542ULL, 5),  Magic(553663774767923222ULL, 6),
    Magic(12762156835873452058ULL, 5),  Magic(8697719377610935353ULL, 5),  Magic(18214094224966588419ULL, 5),
    Magic(5284047716332127234ULL, 5),   Magic(2958538565638195718ULL, 5),  Magic(2218322021241343082ULL, 5),
    Magic(17671582234458915585ULL, 6),
};

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

void GenerateRookMoves()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = rookMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic& magic = rookMagics[s];

        magic.moves = new Bitboard[1 << magic.offset];

        memset(magic.moves, 0, 1 << magic.offset);

        for (unsigned int i = 0; i < numBlockers; i++)
        {
            Bitboard blocker = getBlockers(blockerMask, i);

            int index = (magic.magic * blocker) >> (64 - magic.offset);

            magic.moves[index] = sendRay(s, NORTH, blocker) | sendRay(s, SOUTH, blocker) | sendRay(s, EAST, blocker) |
                                 sendRay(s, WEST, blocker);
        }
    }
}

void GenerateBishopMoves()
{
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = bishopMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic& magic = bishopMagics[s];

        magic.moves = new Bitboard[1 << magic.offset];
        memset(magic.moves, 0, 1 << magic.offset);

        for (unsigned int i = 0; i < numBlockers; i++)
        {
            Bitboard blocker = getBlockers(blockerMask, i);

            int index = (magic.magic * blocker) >> (64 - magic.offset);

            magic.moves[index] = sendRay(s, NORTH_EAST, blocker) | sendRay(s, SOUTH_WEST, blocker) |
                                 sendRay(s, SOUTH_EAST, blocker) | sendRay(s, NORTH_WEST, blocker);
        }
    }
}

void InitMagics()
{
    InitNoEdgeMasks();
    GenerateBishopMoves();
    GenerateRookMoves();
}

#ifdef MAGIC_GEN

unsigned long long RandNum()
{
    static std::mt19937_64 rng((unsigned long long)time(nullptr));
    return rng();
}

unsigned long long getMagicSize(Magic* magics)
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

bool TestMagic(Key magicNum, Bitboard* list, unsigned int size, unsigned short numBits)
{
    bool* test = new bool[1 << numBits];

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
            unsigned long long magicNum = RandNum();

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

            magicNum = RandNum();

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