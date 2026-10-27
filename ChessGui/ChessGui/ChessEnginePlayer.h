#pragma once
#include "Game.h"
#include "UciConnector.h"
#include "EngineLevel.h"

namespace Chess {

	enum class PositionMode { StartPos, PureFen, FenWindow };
	constexpr int FenWindowSize = 8;

	class ChessEnginePlayer
	{
		GameAptr _game;
		UciConnectorAPtr _connector;
		EngineLevel _level;
		PositionMode _mode;

		void StartEngine();

	public:
		ChessEnginePlayer(const GameAptr& game,
		                  EngineLevel level = EngineLevel{3},
		                  PositionMode mode = PositionMode::FenWindow);

		void DoMove();
		void KillEngine();

		// Position, move list and engine state as text, to log when a move request fails.
		std::string DebugDump();
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
