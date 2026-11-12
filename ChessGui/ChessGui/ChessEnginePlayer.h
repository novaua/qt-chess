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
		// startNow false: the engine starts with the first FindMove(), after SetCancelCheck(), so that
		// KillEngine() and the cancel check can end its start too (BL-16 review: the start in the
		// constructor couldn't be stopped).
		ChessEnginePlayer(EngineLevel level = EngineLevel{3},
		                  PositionMode mode = PositionMode::FenWindow, bool startNow = true);

		// The engine's move in UCI notation (e.g. "e7e8q") for the game with these moves
		// from the start position. maxMoveTime: the robot's clock caps the level's thinking time (0: no cap).
		std::string FindMove(const MovesHistory& history, std::chrono::milliseconds maxMoveTime = {});
		// Stops the engine process; thread-safe.
		void KillEngine();
		// A request that is no longer wanted (the game was left): FindMove checks this before and after
		// it starts (or restarts) the engine, before it searches.
		void SetCancelCheck(std::function<bool()> cancelled) { _cancelled = std::move(cancelled); }

		// Position, move list and engine state as text, to log when a move request fails.
		std::string DebugDump(const MovesHistory& history);
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
