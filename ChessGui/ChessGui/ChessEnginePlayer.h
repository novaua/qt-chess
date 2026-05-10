#pragma once
#include "Game.h"
#include "UciConnector.h"

namespace Chess {

	class ChessEnginePlayer
	{
		GameAptr _game;
		UciConnectorAPtr _connector;

	public:
		ChessEnginePlayer(const GameAptr& game, int difficulty = 10);

		void DoMove();
		void KillEngine();

private:
	int _difficulty;
	};

	typedef std::shared_ptr<ChessEnginePlayer> ChessEnginePlayerAptr;
}
