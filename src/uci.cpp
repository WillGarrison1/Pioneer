#include "uci.h"

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
        std::cout << "PioneerV4.0> ";
        std::getline(std::cin, input);
        std::stringstream parse(input);

        parse >> word;

        if (input == "quit")
            break;

        else if (input == "uci")
            std::cout << "id name PioneerV4.0\n"
                      << "id author Pioneer\n"
                      << "uciok\n";

        else if (input == "isready")
            std::cout << "readyok\n";

        else if (input == "d")
            engine.print();

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
                int depth, nodes;
                do
                {
                    if (word == "depth")
                    {
                        parse >> word;
                        depth = atoi(word.c_str());
                    }
                } while (parse >> word);

                engine.go(depth);
            }
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