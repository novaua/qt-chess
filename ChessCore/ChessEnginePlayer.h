#pragma once
#include "Game.h"
#include "UciConnector.h"

namespace Chess {

	class ChessEnginePlayer
	{
		GameAptr _game;
		UciConnectorAPtr _connector;

	public:
		ChessEnginePlayer(const GameAptr& game);

		void DoMove();
	};
}
