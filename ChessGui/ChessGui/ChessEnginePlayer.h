#pragma once
#include <functional>
#include "Game.h"
#include "UciConnector.h"
#include "EngineLevel.h"

namespace Chess {

	enum class PositionMode { StartPos, PureFen, FenWindow };
	constexpr int FenWindowSize = 8;

	// Asks the engine for a move. It never touches the app's Game: it gets a copy of the
	// move list and returns the move, which the caller plays on its own thread (BL-09).
	class ChessEnginePlayer
	{
		UciConnectorAPtr _connector;
		EngineLevel _level;
		PositionMode _mode;
		std::mt19937 _rng{ std::random_device{}() };
		std::function<bool()> _cancelled; // set by the worker: true when the request is abandoned

		void StartEngine();

	public:
		ChessEnginePlayer(EngineLevel level = EngineLevel{3},
		                  PositionMode mode = PositionMode::FenWindow);

		// The engine's move in UCI notation (e.g. "e7e8q") for the game with these moves
		// from the start position.
		std::string FindMove(const MovesHistory& history);
		// Stops the engine process; thread-safe.
		void KillEngine();
		// A request that is no longer wanted (the game was left): FindMove checks this after it
		// restarts a dead engine, before it searches.
		void SetCancelCheck(std::function<bool()> cancelled) { _cancelled = std::move(cancelled); }

		// Position, move list and engine state as text, to log when a move request fails.
		std::string DebugDump(const MovesHistory& history);
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
