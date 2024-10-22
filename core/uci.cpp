// Handles functions between the user and the engine relating to the UCI protocol

#include <iostream>
#include <sstream>
#include <chrono>

#include "uci.h"
#include "search/evaluate.h"
#include "search/moveOrder.h"
#include "representation/bitboard.h"
#include "movegen/movegen.h"



using namespace std;

Board board;

void setup()
{
    initDirections();
    initBBs();
    Zobrist::setup();

    // Initialize magic bitboards

    precomputeRookMoves();
    precomputeBishopMoves();


    board = Board(); // Initialize the board after the magic bitboards are set up
    board.loadFEN(startFen, true, true, true, true, true, -1);
}

Move stringToMove(string moveString, Board board)
{
    bool castle = false;
    Piece promote = Pieces::Empty;
    int to;
    int from;
    if ((moveString == "e1g1" || moveString == "e8g8") && (Pieces::getType(board.board[4]) == Pieces::King || Pieces::getType(board.board[60]) == Pieces::King))
    {
        from = board.isWhite ? 60 : 4;
        to = board.isWhite ? 62 : 6;
        castle = true;
    }
    else if ((moveString == "e1c1" || moveString == "e8c8") && (Pieces::getType(board.board[4]) == Pieces::King || Pieces::getType(board.board[60]) == Pieces::King))
    {
        from = board.isWhite ? 60 : 4;
        to = board.isWhite ? 58 : 2;
        castle = true;
    }
    else if (moveString.length() < 4)
    {
        return 0;
    }
    else
    {
        from = ((moveString[0] - 'a') + (8 * (7 - (moveString[1] - '1'))));
        to = ((moveString[2] - 'a') + (8 * (7 - (moveString[3] - '1'))));
    }
    if (moveString.length() == 5)
    {
        promote = Pieces::charToPiece(moveString[4]);
    }

    return board.getMove(from, to, promote, castle);
}

string moveToString(Move move)
{
    string moveString = "";
    if (isCastle(move))
    {
        if (getTo(move) == 62)
        {
            moveString = "e1g1";
        }
        else if (getTo(move) == 58)
        {
            moveString = "e1c1";
        }
        else if (getTo(move) == 6)
        {
            moveString = "e8g8";
        }
        else if (getTo(move) == 2)
        {
            moveString = "e8c8";
        }
        return moveString;
    }
    moveString += (char)(indexToFile(move & 0x3F) + 'a');
    moveString += (char)(7 - indexToRank(move & 0x3F) + '1');
    moveString += (char)(indexToFile(move >> 6 & 0x3F) + 'a');
    moveString += (char)(7 - indexToRank(move >> 6 & 0x3F) + '1');

    if (isPromotion(move))
    {
        moveString += Pieces::pieceToChar(getPromotion(move) | Pieces::Black);
    }

    return moveString;
}

char pieceToChar(Piece piece)
{
    switch (piece)
    {
    case Pieces::White | Pieces::Pawn:
        return 'P';
    case Pieces::White | Pieces::Knight:
        return 'N';
    case Pieces::White | Pieces::Bishop:
        return 'B';
    case Pieces::White | Pieces::Rook:
        return 'R';
    case Pieces::White | Pieces::Queen:
        return 'Q';
    case Pieces::White | Pieces::King:
        return 'K';
    case Pieces::Black | Pieces::Pawn:
        return 'p';
    case Pieces::Black | Pieces::Knight:
        return 'n';
    case Pieces::Black | Pieces::Bishop:
        return 'b';
    case Pieces::Black | Pieces::Rook:
        return 'r';
    case Pieces::Black | Pieces::Queen:
        return 'q';
    case Pieces::Black | Pieces::King:
        return 'k';
    case Pieces::Empty:
        return 'E';
    default:
        return 'U';
    }
}

void parseUCI(istringstream &parser)
{
    cout << "id name Pioneer V0.3.2\n";
    cout << "id author Will Garrison\n";
    cout << "uciok\n";
}

void parseIsReady(istringstream &parser)
{

    cout << "readyok" << endl;
}

void parseNewGame(istringstream &parser)
{
}

void parsePosition(istringstream &parser)
{
    string input;
    parser >> input;

    if (input == "startpos")
    {
        board.loadFEN(startFen, true, true, true, true, true, -1);
    }
    else if (input == "fen")
    {

        parser >> input;
        string fen = input;
        parser >> input;
        bool isWhite = input == "w";
        parser >> input;
        bool whiteCanCastleKingSide = false;
        bool whiteCanCastleQueenSide = false;
        bool blackCanCastleKingSide = false;
        bool blackCanCastleQueenSide = false;

        for (auto x : input)
        {
            if (x == 'Q')
            {
                whiteCanCastleQueenSide = true;
            }
            else if (x == 'K')
            {
                whiteCanCastleKingSide = true;
            }
            else if (x == 'q')
            {
                blackCanCastleQueenSide = true;
            }
            else if (x == 'k')
            {
                blackCanCastleKingSide = true;
            }
        }
        parser >> input;
        int enPassantSquare = -1;
        if (input != "-")
        {
            enPassantSquare = ((input[0] - 'a') + (8 * (7 - (input[1] - '1'))));
        }
        board.loadFEN(fen, isWhite, whiteCanCastleKingSide, whiteCanCastleQueenSide, blackCanCastleKingSide, blackCanCastleQueenSide, -1);
    }
    string moveS;
    while (parser >> moveS)
    {
        if (moveS == "moves")
        {
            break;
        }
    }
    if (moveS == "moves")
    {
        while (parser >> moveS)
        {
            Move move = stringToMove(moveS, board);
            board.makeMove(move);
        }
    }
}

// Starts a search by the engine based off the specified parameters
// TODO: Implement wtime and btime
void parseGo(istringstream &parser)
{
    bool perft = false;             // Is a perft search
    unsigned int depthValue = MAX_DEPTH;  // Targeted depth of the search
    unsigned int nodesCount = 0;    // Max number of nodes to search
    unsigned int moveTimeValue = 0; // Max time to search in milliseconds
    unsigned int wtime = 0;         // White time
    unsigned int btime = 0;         // Black time

    while (parser)
    {
        string option;
        parser >> option;
        if (option == "perft")
        {
            // Is a perft search
            perft = true;

            // Set depth of perft search
            string depthInput;
            parser >> depthInput;
            depthValue = stoi(depthInput);
        }
        else if (option == "depth")
        {
            // User specified depth
            string depthInput;
            parser >> depthInput;
            depthValue = stoi(depthInput);
        }
        else if (option == "nodes")
        {
            // User specified number of nodes
            string nodesInput;
            parser >> nodesInput;
            nodesCount = stoi(nodesInput);
        }
        else if (option == "movetime")
        {
            // User specified time to search
            string moveTimeInput;
            parser >> moveTimeInput;
            moveTimeValue = stoi(moveTimeInput);
        }
        else if (option == "wtime")
        {
            string wtimeInput;
            parser >> wtimeInput;
            wtime = stoi(wtimeInput);
        }
        else if (option == "btime")
        {
            string btimeInput;
            parser >> btimeInput;
            btime = stoi(btimeInput);
        }
    }
    if (perft)
    {
        // Perft search
        cout << "\n";

        auto start = chrono::high_resolution_clock::now();
        unsigned long long perft = startPerft(board, depthValue);
        auto stop = chrono::high_resolution_clock::now();

        cout << "Perft search to depth: " << depthValue << "\n"
             << "Took " << chrono::duration_cast<chrono::milliseconds>(stop - start).count() << "ms\n";
        cout << "Nodes: " << perft << "\n";
    }
    else
    {
        // Normal search
        auto start = chrono::high_resolution_clock::now();
        Move bestMove = startSearch(&board, depthValue, moveTimeValue, nodesCount, wtime, btime);
        auto stop = chrono::high_resolution_clock::now();
        cout << "bestmove " << moveToString(bestMove) << endl;
    }
}

void parseEval(istringstream &parser)
{
    cout << "Eval: " << evaluate(&board) << "\n";
}

void parseMakeMove(istringstream &parser)
{
    string input;
    parser >> input;
    Move move = stringToMove(input, board);
    board.makeMove(move);
}

void parseClearTT(istringstream &parser)
{
    clearTTSearch();
}

void parseUndoMove(istringstream &parser)
{
    board.undoMove();
}

void parseDebug(istringstream &parser)
{
    string option;
    parser >> option;
    if (option == "print")
    {
        string input;
        parser >> input;
        if (input == "bitboard")
        {
            for (auto x : board.pieceBB)
            {
                printBitboard(&x);
            }
            printBitboard(&board.allPiecesBB);
            printBitboard(&board.colorBB[0]);
            printBitboard(&board.colorBB[8]);
        }
        if (input == "enpassant")
        {
            cout << board.enPassantSquare << endl;
        }
        if (input == "attackedBB")
        {
            Bitboard attackedBB = board.getAttackedBB(board.sideToMove);
            printBitboard(&attackedBB);
        }
    }
}

void parseDisplay(istringstream &parser)
{
    board.printBoard();
    cout << "\n";
    board.printFEN();
}
