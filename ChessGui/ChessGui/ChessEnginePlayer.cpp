#include "stdafx.h"
#include "ChessEnginePlayer.h"

using namespace Chess;

ChessEnginePlayer::ChessEnginePlayer(const GameAptr& game, int difficulty)
	: _game(game), _difficulty(difficulty)
{
	_connector = std::make_shared<UciConnector>();
	_connector->Init();
	_connector->SetDifficulty(_difficulty);
}

void ChessEnginePlayer::DoMove()
{
	auto moveRequest = StartPosMoveRequest();
	auto history = _game->GetGameRecord();

	for (auto move : history) {
		moveRequest.Moves.push_back(move.ToUciString());
	}

	auto move = _connector->GetEngineMove(moveRequest, std::chrono::seconds(1));

	_game->DoMove(Move::Parse(move.BestMove));
}

void ChessEnginePlayer::KillEngine()
{
	_connector->Kill();
}
