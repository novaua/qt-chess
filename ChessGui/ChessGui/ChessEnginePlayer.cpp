#include "stdafx.h"
#include "ChessEnginePlayer.h"
#include "ChessException.h"

using namespace Chess;

ChessEnginePlayer::ChessEnginePlayer(EngineLevel level, PositionMode mode)
	: _level(level), _mode(mode)
{
	_connector = std::make_shared<UciConnector>();
	StartEngine();
}

void ChessEnginePlayer::StartEngine()
{
	_connector->Init();
	if (_level.UsesEngineElo())
		_connector->SetDifficulty(_level.Elo());
}

// A pawn that reached the last rank but has no piece chosen yet. Sent to the engine
// as "e7e8" it is an illegal move, which makes Stockfish 18+ exit.
static bool IsUnresolvedPromotion(const HistoryMove& m)
{
	auto to = static_cast<int>(m.To.Position);
	return m.From.Piece.Type == PAWN && (to < 8 || to >= 56) && m.PromotedTo.IsEmpty();
}

// A game with the first `count` moves of `history`, played from the start position.
static GameAptr Replay(const MovesHistory& history, size_t count)
{
	auto game = std::make_shared<Game>();
	game->Restart();
	for (size_t i = 0; i < count && i < history.size(); ++i)
		game->DoMove(history[i].ToMove());
	return game;
}

std::string ChessEnginePlayer::FindMove(const MovesHistory& history)
{
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
		req.Fen = Replay(history, n)->MakeFen();
	}
	else if (_mode == PositionMode::FenWindow && n > FenWindowSize)
	{
		int snapshotAt = (n / FenWindowSize) * FenWindowSize;

		req.Fen = Replay(history, snapshotAt)->MakeFen();

		for (int i = snapshotAt; i < n; ++i)
			req.Moves.push_back(history[i].ToUciString());
	}
	else
	{
		for (const auto& m : history)
			req.Moves.push_back(m.ToUciString());
	}

	if (_level.UsesEngineElo())
		return _connector->GetEngineMove(req, _level.MoveTime()).BestMove;

	auto candidates = _connector->GetCandidateMoves(req, _level.CandidateCount(), _level.SearchDepth());
	return PickWeakMove(candidates, _level.TemperatureCp(), _rng);
}

void ChessEnginePlayer::KillEngine()
{
	_connector->Kill();
}

std::string ChessEnginePlayer::DebugDump(const MovesHistory& history)
{
	static const char* modeNames[] = { "StartPos", "PureFen", "FenWindow" };
	auto game = Replay(history, history.size());

	std::string out = std::string("mode: ") + modeNames[static_cast<int>(_mode)]
		+ ", level " + std::to_string(_level.Level());
	if (_level.UsesEngineElo())
		out += ", Elo " + std::to_string(_level.Elo())
			+ ", movetime " + std::to_string(_level.MoveTime().count()) + " ms\n";
	else
		out += ", weak picker: " + std::to_string(_level.CandidateCount()) + " candidates"
			+ ", depth " + std::to_string(_level.SearchDepth())
			+ ", temperature " + std::to_string(_level.TemperatureCp()) + " cp\n";
	out += std::string("side to move: ") + (game->IsWhiteMove() ? "white" : "black")
		+ ", plies played: " + std::to_string(history.size()) + "\n";
	out += "fen: " + game->MakeFen() + "\n";

	out += "moves:";
	for (const auto& m : history)
		out += " " + m.ToUciString();
	out += "\n";

	out += "board (white at the bottom, '.' = empty):\n";
	for (int rank = 7; rank >= 0; --rank) {
		out += "  " + std::to_string(rank + 1) + " ";
		for (int file = 0; file < 8; ++file) {
			auto piece = game->GetPieceAt(rank * 8 + file);
			out += piece.IsEmpty() ? "." : piece.ToString();
			out += " ";
		}
		out += "\n";
	}
	out += "    a b c d e f g h\n";

	return out + _connector->DebugState();
}
