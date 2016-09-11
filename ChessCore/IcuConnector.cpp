#include "stdafx.h"
#include "IcuConnector.h"
#include "Utils.h"

using namespace Chess;
using namespace std;

IcuConnector::IcuConnector(std::istream &istr, std::ostream &ostr)
	:_istr(istr), _ostr(ostr)
{
}

IcuConnector::~IcuConnector()
{
}

void Chess::IcuConnector::Init()
{
	_ostr << "icu" << endl;
}

void Chess::IcuConnector::StartPosition()
{
	_ostr << "position startpos" << endl;
}

bool Chess::IcuConnector::IsReady()
{
	_ostr << "isready" << endl;
	std::string line;
	getline(_istr, line);
	return (line == "readyok");
}

void Chess::IcuConnector::PositionMoves(Move move)
{
	_ostr << "position moves " << move.From << move.To << endl;
}

std::pair<Move, Move> IcuConnector::Go()
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
