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

	public:
		ChessEnginePlayer(const GameAptr& game,
		                  EngineLevel level = EngineLevel{3},
		                  PositionMode mode = PositionMode::FenWindow);

		void DoMove();
		void KillEngine();
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
