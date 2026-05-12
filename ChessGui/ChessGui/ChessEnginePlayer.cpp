#include "stdafx.h"
#include "ChessEnginePlayer.h"
#include "ChessException.h"

using namespace Chess;

ChessEnginePlayer::ChessEnginePlayer(const GameAptr& game, EngineLevel level)
	: _game(game), _level(level)
{
	_connector = std::make_shared<UciConnector>();
	_connector->Init();
	_connector->SetDifficulty(_level.SkillLevel());
}

void ChessEnginePlayer::DoMove()
{
	auto moveRequest = StartPosMoveRequest();
	auto history = _game->GetGameRecord();

	for (auto move : history) {
		moveRequest.Moves.push_back(move.ToUciString());
	}

	auto moveResponse = _connector->GetEngineMove(moveRequest, _level.MoveTime());
	auto chessMove = Move::Parse(moveResponse.BestMove);
	_game->DoMove(chessMove);
}

void ChessEnginePlayer::KillEngine()
{
	_connector->Kill();
}
