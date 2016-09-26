#include "stdafx.h"
#include "UciConnector.h"
#include "Utils.h"

using namespace Chess;
using namespace std;

#define MAX_SUPPRESSED_LINES    10

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

bool Chess::UciConnector::IsReady()
{
    std::string line;
    auto result = false;

    _ostr << "isready" << endl;

    for (int i = 0; !result && i < MAX_SUPPRESSED_LINES; ++i)
    {
        if (getline(_istr, line)) {
            result = match_command("readyok", line);
        }
    }

    return result;
}

void Chess::UciConnector::PositionMoves(Move move)
{
    _ostr << "position moves " << move.From << move.To << endl;
}

std::pair<Move, Move> UciConnector::Go()
{
    _ostr << "go" << endl;

    std::pair<Move, Move> resut = std::make_pair(Move::Empty, Move::Empty);
    for (int i = 0; i < MAX_SUPPRESSED_LINES; ++i)
    {
        std::string line;
        if (getline(_istr, line)) {
            if (match_command("bestmove", line))
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
