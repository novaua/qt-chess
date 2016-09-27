#include "stdafx.h"
#include "UciBasicConnector.h"

using namespace Chess;

std::string ExePath = "D:\\tools\\stockfish-7-win\\Windows\\stockfish\ 7\ 32bit.exe";

UciBasicConnector::UciBasicConnector()
    :_engine(ExePath),
    _connector(_engine, _engine)
{
    _connector.Init();
    _connector.StartPosition();
}


UciBasicConnector::~UciBasicConnector()
{
}

std::pair<Move, Move> UciBasicConnector::MoveTo(const Move &move)
{
    _connector.IsReady();
    _connector.PositionMoves(move);
    return _connector.Go();
}

void UciBasicConnector::NewGame()
{
    _connector.StartPosition();
}
