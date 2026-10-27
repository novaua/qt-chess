#include "stdafx.h"
#include "ChessEnginePlayer.h"
#include "ChessException.h"

using namespace Chess;

ChessEnginePlayer::ChessEnginePlayer(const GameAptr& game, EngineLevel level, PositionMode mode)
	: _game(game), _level(level), _mode(mode)
{
	_connector = std::make_shared<UciConnector>();
	StartEngine();
}

void ChessEnginePlayer::StartEngine()
{
	_connector->Init();
	_connector->SetDifficulty(_level.Elo());
}

// A pawn that reached the last rank but has no piece chosen yet. Sent to the engine
// as "e7e8" it is an illegal move, which makes Stockfish 18+ exit.
static bool IsUnresolvedPromotion(const HistoryMove& m)
{
	auto to = static_cast<int>(m.To.Position);
	return m.From.Piece.Type == PAWN && (to < 8 || to >= 56) && m.PromotedTo.IsEmpty();
}

void ChessEnginePlayer::DoMove()
{
	auto history = _game->GetGameRecord();
	int n = static_cast<int>(history.size());

	if (n > 0 && IsUnresolvedPromotion(history.back()))
		throw std::logic_error("Pawn promotion piece has not been chosen yet");

	// The engine process may have died on an earlier request; start a fresh one
	// instead of failing every following move with an empty response.
	if (!_connector->IsRunning())
		StartEngine();

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
