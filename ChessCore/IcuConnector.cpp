#include "stdafx.h"
#include "IcuConnector.h"
using namespace Chess;
using namespace std;

IcuConnector::IcuConnector(std::istream &istr, std::ostream &ostr)
	:_istr(istr), _ostr(_ostr)
{
}


IcuConnector::~IcuConnector()
{
}

void Chess::IcuConnector::Init()
{
	_ostr << "icu" << endl;
	StartPosition();
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
	_ostr << "position moves " << endl;
}

std::pair<Move, Move> IcuConnector::Go()
{
	std::pair<Move, Move> resut({ e2, e4 }, { e2, e4 });
	return resut;
}
