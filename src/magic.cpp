
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

Bitboard* rookMoves;
Bitboard* bishopMoves;

// Rook magic (800kb)
Magic rookMagics[] = {
    Magic(36029071914967170ULL, 12),    Magic(4972009447870202112ULL, 11), Magic(4683778806506389640ULL, 11),
    Magic(2341889407344705664ULL, 11),  Magic(7061697027173517824ULL, 11), Magic(720593571421882880ULL, 11),
    Magic(612490648842406400ULL, 11),   Magic(36029896535844992ULL, 12),   Magic(140739640033573ULL, 11),
    Magic(11529918870957604864ULL, 10), Magic(1153203049378873604ULL, 10), Magic(578853736290488320ULL, 10),
    Magic(292875005350385664ULL, 10),   Magic(36169543097516160ULL, 10),   Magic(1125904272066594ULL, 10),
    Magic(72620546145256449ULL, 11),    Magic(44530225120384ULL, 11),      Magic(153125136650731664ULL, 10),
    Magic(666674032363495425ULL, 10),   Magic(4613976301299126272ULL, 10), Magic(2595481859828219921ULL, 10),
    Magic(563499776376834ULL, 10),      Magic(21990502569218ULL, 10),      Magic(144117387136935044ULL, 11),
    Magic(9304578675739592768ULL, 11),  Magic(292747445870272512ULL, 10),  Magic(580544295346176ULL, 10),
    Magic(4688247285108711682ULL, 10),  Magic(40686225719312ULL, 10),      Magic(432347765398569088ULL, 10),
    Magic(14411519924286918712ULL, 10), Magic(9547640701903717385ULL, 11), Magic(1188951813726928964ULL, 11),
    Magic(20125873294295044ULL, 10),    Magic(9376776036617830416ULL, 10), Magic(149540032221186ULL, 10),
    Magic(9233505685981890560ULL, 10),  Magic(2382967376204269584ULL, 10), Magic(14447574199109171458ULL, 10),
    Magic(1171312777102596ULL, 11),     Magic(583252160825819136ULL, 11),  Magic(9804336526270676992ULL, 10),
    Magic(54892852265811984ULL, 10),    Magic(865863208391540752ULL, 10),  Magic(2666975507413794820ULL, 10),
    Magic(2738751557821661200ULL, 10),  Magic(9225069691960295440ULL, 10), Magic(4758336140241666068ULL, 11),
    Magic(3171308194416820608ULL, 11),  Magic(18015551722897664ULL, 10),   Magic(4616194566393992320ULL, 10),
    Magic(9016278851277312ULL, 10),     Magic(4625215680814579968ULL, 10), Magic(144363679851479168ULL, 10),
    Magic(9295438847918736384ULL, 10),  Magic(2306055786461399552ULL, 11), Magic(9367858864164184130ULL, 12),
    Magic(7649434336554389635ULL, 11),  Magic(18155823596323330ULL, 11),   Magic(1299297719713533953ULL, 11),
    Magic(9228157130820288561ULL, 11),  Magic(146929972893717522ULL, 11),  Magic(585488020062864644ULL, 11),
    Magic(40674796355207206ULL, 12),
};

// Bishop magics (41kb)
Magic bishopMagics[] = {
    Magic(18300276130055184ULL, 6),    Magic(9369213485063471108ULL, 5), Magic(45318712597291016ULL, 5),
    Magic(23696958071246848ULL, 5),    Magic(9223937254563512458ULL, 5), Magic(219270244094443521ULL, 5),
    Magic(607532053694496ULL, 5),      Magic(81207734109736028ULL, 6),   Magic(35219822412800ULL, 5),
    Magic(144259292852685058ULL, 5),   Magic(5767004492776098320ULL, 5), Magic(325459857056464960ULL, 5),
    Magic(2256215576953345ULL, 5),     Magic(142970939244556ULL, 5),     Magic(19147462690735106ULL, 5),
    Magic(10955008869269505024ULL, 5), Magic(594475219800948864ULL, 5),  Magic(324839749804692480ULL, 5),
    Magic(166638461511696640ULL, 7),   Magic(1136998171543552ULL, 7),    Magic(3459890424475287600ULL, 7),
    Magic(577024599914580760ULL, 7),   Magic(2306124502528434272ULL, 5), Magic(13835339531349923588ULL, 5),
    Magic(2312625477762699328ULL, 5),  Magic(1155481455708608784ULL, 5), Magic(299067448230048ULL, 7),
    Magic(10133375180427648ULL, 9),    Magic(5782904498180603914ULL, 9), Magic(18579547490878016ULL, 7),
    Magic(1154047954286829826ULL, 5),  Magic(9225907648392628225ULL, 5), Magic(76652556211734528ULL, 5),
    Magic(3033185412869915652ULL, 5),  Magic(290279660782592ULL, 7),     Magic(360290171360903552ULL, 9),
    Magic(1127000225087552ULL, 9),     Magic(9232380062891835776ULL, 7), Magic(565209142133376ULL, 5),
    Magic(1162192414064992ULL, 5),     Magic(37172297859076418ULL, 5),   Magic(18722488839047296ULL, 5),
    Magic(9229283294867112001ULL, 7),  Magic(288266943511791620ULL, 7),  Magic(18023512566400000ULL, 7),
    Magic(522435166411424192ULL, 7),   Magic(622631753818834177ULL, 5),  Magic(9423946048612598913ULL, 5),
    Magic(1153392128835856ULL, 5),     Magic(1171078910530551808ULL, 5), Magic(2200132158017ULL, 5),
    Magic(9246031048465317888ULL, 5),  Magic(2392606561665024ULL, 5),    Magic(1731101927724351488ULL, 5),
    Magic(18090350720192512ULL, 5),    Magic(4940459794998764620ULL, 5), Magic(9233787203707355144ULL, 6),
    Magic(2377902822961711232ULL, 5),  Magic(184647585460520972ULL, 5),  Magic(4611686164460601872ULL, 5),
    Magic(1207951235875344ULL, 5),     Magic(6755438368590350ULL, 5),    Magic(3415100362892288ULL, 5),
    Magic(599295418388578320ULL, 6),
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

    unsigned long long size = 0;

    for (Magic m : rookMagics)
    {
        size += 1 << m.offset;
    }

    std::cout << "Rook magic size: " << (unsigned long long)((float)size / 1024.0f * 8.0f) << "kb" << std::endl;

    rookMoves = new Bitboard[size];

    Bitboard* pointer = rookMoves;
    memset(rookMoves, 0, size * 8.0f);
    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = rookMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic& magic = rookMagics[s];

        magic.moves = pointer;
        pointer += 1 << magic.offset;

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
    unsigned long long size = 0;

    for (Magic m : bishopMagics)
    {
        size += 1 << m.offset;
    }

    std::cout << "Bishop magic size: " << (unsigned long long)((float)size / 1024.0f * 8.0f) << "kb" << std::endl;

    bishopMoves = new Bitboard[size];

    Bitboard* pointer = bishopMoves;
    memset(bishopMoves, 0, size * 8.0f);

    for (Square s = SQ_A1; s <= SQ_H8; s++)
    {
        Bitboard blockerMask = bishopMasks[s];

        blockerMask &= noEdgeMask[s];
        unsigned int numBlockers = 1 << popCount(blockerMask);

        Magic& magic = bishopMagics[s];

        magic.moves = pointer;
        pointer += 1 << magic.offset;

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