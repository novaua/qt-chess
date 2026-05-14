#include "stdafx.h"
#include "ChessEnginePlayer.h"
#include "ChessException.h"

using namespace Chess;

ChessEnginePlayer::ChessEnginePlayer(const GameAptr& game, EngineLevel level, PositionMode mode)
	: _game(game), _level(level), _mode(mode)
{
	_connector = std::make_shared<UciConnector>();
	_connector->Init();
	_connector->SetDifficulty(_level.SkillLevel());
}

void ChessEnginePlayer::DoMove()
{
	auto history = _game->GetGameRecord();
	int n = static_cast<int>(history.size());
	StartPosMoveRequest req;

	if (_mode == PositionMode::PureFen)
	{
		req.Fen = _game->MakeFen();
	}
	else if (_mode == PositionMode::FenWindow && n > FenWindowSize)
	{
		int snapshotAt = (n / FenWindowSize) * FenWindowSize;

		auto snap = std::make_shared<Game>();
		snap->Restart();
		for (int i = 0; i < snapshotAt; ++i)
			snap->DoMove(history[i].ToMove());
		req.Fen = snap->MakeFen();

		for (int i = snapshotAt; i < n; ++i)
			req.Moves.push_back(history[i].ToUciString());
	}
	else
	{
		for (const auto& m : history)
			req.Moves.push_back(m.ToUciString());
	}

	auto moveResponse = _connector->GetEngineMove(req, _level.MoveTime());
	_game->DoMove(Move::Parse(moveResponse.BestMove));
}

void ChessEnginePlayer::KillEngine()
{
	_connector->Kill();
}
