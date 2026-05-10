#pragma once
#include "Game.h"
#include "UciConnector.h"
#include "EngineLevel.h"

namespace Chess {

	class ChessEnginePlayer
	{
		GameAptr _game;
		UciConnectorAPtr _connector;
		EngineLevel _level;

	public:
		ChessEnginePlayer(const GameAptr& game, EngineLevel level = EngineLevel{3});

		void DoMove();
		void KillEngine();
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
