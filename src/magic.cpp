
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
    Magic(12187243722325731611ULL, 14), Magic(14052078531810250780ULL, 12), Magic(15548745779470730325ULL, 13),
    Magic(16621301764145226556ULL, 13), Magic(11712220029930996394ULL, 13), Magic(1008802554006680072ULL, 12),
    Magic(5307480314069970161ULL, 12),  Magic(3362984213025876968ULL, 13),  Magic(6897453273229237517ULL, 12),
    Magic(17115441393039710210ULL, 11), Magic(7077762271535456729ULL, 11),  Magic(15670092325215042680ULL, 11),
    Magic(10127868875945677461ULL, 11), Magic(10513653999878093063ULL, 11), Magic(16221740262868846467ULL, 11),
    Magic(5315104967801535172ULL, 12),  Magic(16487711638579323712ULL, 12), Magic(16856565010956364173ULL, 11),
    Magic(12364877151574931800ULL, 11), Magic(2793185664927354605ULL, 11),  Magic(1719103453141845749ULL, 11),
    Magic(2366659127620620119ULL, 11),  Magic(15065948058812314389ULL, 11), Magic(9782265138229066706ULL, 12),
    Magic(15511457614804674242ULL, 12), Magic(10499412052139977596ULL, 11), Magic(10141891278001345046ULL, 11),
    Magic(9471678144690859455ULL, 11),  Magic(10326383888760680492ULL, 11), Magic(13916686967738359082ULL, 12),
    Magic(18404221194619967894ULL, 11), Magic(12730377907923410903ULL, 12), Magic(9873456579829830067ULL, 12),
    Magic(8479130186219085610ULL, 11),  Magic(8593210979289920128ULL, 11),  Magic(12152067484303259594ULL, 11),
    Magic(11598397143391411533ULL, 11), Magic(14697335598804168132ULL, 11), Magic(3952599250429808788ULL, 11),
    Magic(5074008711756777728ULL, 11),  Magic(3399029152977627249ULL, 12),  Magic(5670378685356179472ULL, 11),
    Magic(9842164168130313289ULL, 11),  Magic(5231408189919196149ULL, 11),  Magic(13707388145797727101ULL, 11),
    Magic(17004466245796351886ULL, 11), Magic(12282641362046943312ULL, 10), Magic(382742690875323168ULL, 12),
    Magic(3121595948895274496ULL, 12),  Magic(8173315613194880416ULL, 11),  Magic(4740513384283176564ULL, 11),
    Magic(13965876304131969219ULL, 11), Magic(7890466970040328679ULL, 11),  Magic(14222369825534381568ULL, 11),
    Magic(7958182370775142832ULL, 11),  Magic(8841008887894301602ULL, 12),  Magic(12945241208712618458ULL, 13),
    Magic(294778738860753154ULL, 11),   Magic(10789635053253232926ULL, 12), Magic(16534572104527599490ULL, 12),
    Magic(5014714327280653190ULL, 12),  Magic(4917571279327858082ULL, 12),  Magic(9402103436065278220ULL, 12),
    Magic(10900932397893946246ULL, 12),
};
Magic bishopMagics[] = {
    Magic(9754027203531706219ULL, 6),  Magic(5613869010393523274ULL, 5),   Magic(17157637076280978364ULL, 5),
    Magic(15931515065255095364ULL, 5), Magic(8605258505410396283ULL, 5),   Magic(7719456049545538984ULL, 5),
    Magic(4193163640555621347ULL, 5),  Magic(4280232775170395211ULL, 6),   Magic(2187378712201674002ULL, 5),
    Magic(8621888254708479723ULL, 5),  Magic(9286228963796140966ULL, 5),   Magic(17327977942246262142ULL, 5),
    Magic(4435242998344304411ULL, 5),  Magic(15207840267750121781ULL, 5),  Magic(11023521095551406661ULL, 5),
    Magic(14215142603615908905ULL, 5), Magic(12605654251727053870ULL, 5),  Magic(6217256879151059972ULL, 5),
    Magic(412322812635825089ULL, 8),   Magic(2294066559710666633ULL, 8),   Magic(1884122350059398637ULL, 8),
    Magic(18197508724721003198ULL, 8), Magic(11688259620132917215ULL, 5),  Magic(8768630284123263516ULL, 5),
    Magic(10166886124546494406ULL, 5), Magic(1561461960001720018ULL, 5),   Magic(13497876992842729337ULL, 8),
    Magic(97638798437175478ULL, 11),   Magic(14822524817348750688ULL, 11), Magic(8410759277152601310ULL, 7),
    Magic(12098852229539220547ULL, 5), Magic(6860714519753963522ULL, 5),   Magic(18147852853877813291ULL, 5),
    Magic(12710926025742898509ULL, 5), Magic(3497814700050630969ULL, 8),   Magic(17902839209810401571ULL, 11),
    Magic(7630283931754915124ULL, 11), Magic(16017354453330297536ULL, 7),  Magic(4640426728447875105ULL, 5),
    Magic(16082861346003220096ULL, 5), Magic(8858303804514230339ULL, 5),   Magic(13928094365584066769ULL, 5),
    Magic(2149496858186119431ULL, 8),  Magic(12565517899415241451ULL, 8),  Magic(15446990279525007872ULL, 7),
    Magic(5769272434805516649ULL, 8),  Magic(5490015656639626251ULL, 5),   Magic(14906124282412367361ULL, 5),
    Magic(5747450813189797929ULL, 5),  Magic(17382113405762760474ULL, 5),  Magic(975479303264273361ULL, 5),
    Magic(9245487996533747315ULL, 5),  Magic(1617801894124789953ULL, 5),   Magic(3554739857580458420ULL, 5),
    Magic(2121227488900944346ULL, 5),  Magic(11984355718680790986ULL, 5),  Magic(2273131203027348687ULL, 7),
    Magic(1316607321221125264ULL, 5),  Magic(10371463916299751429ULL, 5),  Magic(14378349844589479936ULL, 5),
    Magic(12283859962382296070ULL, 5), Magic(9475905084658001411ULL, 5),   Magic(15997122189944621647ULL, 5),
    Magic(16195137849333891396ULL, 6),
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