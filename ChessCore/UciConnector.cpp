#include "stdafx.h"
#include "UciConnector.h"
#include "Utils.h"

using namespace Chess;
using namespace std;

UciConnector::UciConnector(std::istream &istr, std::ostream &ostr)
    :_istr(istr), _ostr(ostr)
{
}

UciConnector::~UciConnector()
{
}

UciOptionMapRaw Chess::UciConnector::Init()
{
    std::string typeToken = "type";
    std::string optionToken = "option name";

    UciOptionMapRaw options;
    std::string line;

    _ostr << "uci" << endl;
    while (!match_command("uciok", line))
    {
        if (std::getline(_istr, line))
        {
            // remove /r at the end
            line = line.substr(0, line.size() - 1);

            auto isOption = line.find(optionToken);
            if (isOption == std::string::npos)
                continue;

            auto typePos = line.find(typeToken);
            if (typePos == std::string::npos)
                continue;

            auto key = line.substr(optionToken.size() + 1, typePos - optionToken.size() - 2);
            auto value = line.substr(typePos + typeToken.size() + 1);

            options[key] = value;
        }
    }

    return options;
}

void Chess::UciConnector::StartPosition()
{
    _ostr << "position startpos" << endl;
}

void ClearStream(std::istream &istr) {
    while (istr.peek() != EOF)
    {
        std::string line;
        if (std::getline(istr, line))
        {
            std::cout << "cls: " << line << std::endl;
        }
    }
}

bool Chess::UciConnector::IsReady()
{
    ClearStream(_istr);

    _ostr << "isready" << endl;
    std::string line;

    if (getline(_istr, line)) {
        return (begins_with(line, std::string("readyok")));
    }

    return false;
}

void Chess::UciConnector::PositionMoves(Move move)
{
    _ostr << "position moves " << move.From << move.To << endl;
}

std::pair<Move, Move> UciConnector::Go()
{
    std::pair<Move, Move> resut;
    auto bm = std::string("bestmove");
    while (true)
    {
        std::string line;
        getline(_istr, line);
        if (line.size() > bm.size())
        {
            if (line.substr(0, bm.size()) == bm)
            {
                auto v = split(line, ' ');
                auto bestMove = BoardPositionFromString(v[1]);

                if (v.size() > 3)
                {
                    if (v[2] == "ponder")
                    {
                        auto ponder = BoardPositionFromString(v[3]);
                        resut = std::make_pair(FromBoardPositions(bestMove), FromBoardPositions(ponder));
                        break;
                    }
                }

                resut = make_pair(FromBoardPositions(bestMove), Move::Empty);
                break;
            }
        }
    }

    return resut;
}
