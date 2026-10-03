#include "uci.h"

#include "search.h"
#include "transposition.h"
#include "types.h"
#include <iostream>
#include <sstream>
#include <string>

Interface::Interface()
{
}

Interface::~Interface()
{
}

void Interface::run()
{
    std::string input;
    std::string word;
    while (true)
    {
        // Check the stream, not just the contents: on EOF getline fails without ever matching
        // "quit", which previously left this loop spinning on empty input forever.
        if (!std::getline(std::cin, input))
            break;

        std::stringstream parse(input);

        word.clear();
        parse >> word;

        if (input == "quit")
            break;

        else if (input == "uci")
            std::cout << "id name PioneerV4.1\n"
                      << "id author Pioneer\n"
                      << "option name Hash type spin default 64 min 1 max 4096\n"
                      << "uciok" << std::endl;

        else if (word == "setoption")
        {
            // setoption name <id> [value <x>]
            std::string name, value;
            parse >> word; // "name"
            if (word == "name")
            {
                while (parse >> word && word != "value")
                    name += (name.empty() ? "" : " ") + word;
                if (word == "value")
                    while (parse >> word)
                        value += (value.empty() ? "" : " ") + word;
            }

            if (name == "Hash")
                engine.SetHashSize(atoi(value.c_str()));
        }

        else if (input == "isready")
            std::cout << "readyok" << std::endl; // must flush: stdout is block-buffered on a pipe

        else if (input == "ucinewgame")
            engine.ClearTT();

        else if (input == "d")
            engine.print();

        else if (word == "bench")
        {
            unsigned int depth = 0; // 0 -> default
            if (parse >> word)
                depth = atoi(word.c_str());
            engine.bench(depth);
        }

        else if (word == "perftsuite")
            engine.perftSuite();

        else if (word == "position")
        {
            parse >> word;
            if (word == "startpos")
            {
                engine.setFen(START_FEN);

                std::string move;
                parse >> move;
                if (move == "moves")
                {
                    while (parse >> move)
                    {
                        engine.makemove(move);
                    }
                }
            }
            if (word == "fen")
            {
                std::string pos, rest;
                parse >> pos;
                parse >> rest;
                pos += " " + rest;
                parse >> rest;
                pos += " " + rest;
                parse >> rest;
                pos += " " + rest;
                parse >> rest;
                pos += " " + rest;
                parse >> rest;
                pos += " " + rest;

                engine.setFen(pos);

                std::string move;
                parse >> move;
                if (move == "moves")
                {
                    while (parse >> move)
                    {
                        engine.makemove(move);
                    }
                }
            }
        }
        else if (word == "go")
        {
            parse >> word;
            if (word == "perft")
            {
                parse >> word;
                unsigned int depth = atoi(word.c_str());
                engine.goPerft(depth);
            }
            else
            {
                GoParams p{};
                do
                {
                    if (word == "depth")
                    {
                        parse >> word;
                        p.depth = atoi(word.c_str());
                    }
                    else if (word == "movetime")
                    {
                        parse >> word;
                        p.movetime = atoi(word.c_str());
                    }
                    else if (word == "nodes")
                    {
                        parse >> word;
                        p.nodes = atoi(word.c_str());
                    }
                    else if (word == "wtime")
                    {
                        parse >> word;
                        p.wtime = atoi(word.c_str());
                    }
                    else if (word == "btime")
                    {
                        parse >> word;
                        p.btime = atoi(word.c_str());
                    }
                    else if (word == "winc")
                    {
                        parse >> word;
                        p.winc = atoi(word.c_str());
                    }
                    else if (word == "binc")
                    {
                        parse >> word;
                        p.binc = atoi(word.c_str());
                    }
                    else if (word == "movestogo")
                    {
                        parse >> word;
                        p.movestogo = atoi(word.c_str());
                    }
                    else if (word == "infinite")
                    {
                        p.infinite = true;
                    }
                } while (parse >> word);

                engine.go(p);
            }
        }
        else if (word == "stop")
        {
            engine.stop();
        }
        else if (word == "makemove")
        {
            parse >> word;
            Move move(word);
            engine.makemove(move);
        }
        else if (word == "undomove")
            engine.undomove();
        else if (word == "eval")
            engine.eval();
        else if (word == "check")
        {
            parse >> word;
            Move move(word);
            engine.isCheck(move);
        }
    }
}