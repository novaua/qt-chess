#include "Move.h"

namespace Chess
{
	class GameChecks
	{
	public:
		GameChecks(const GameState &state);
		~GameChecks();

		bool IsInCheck(EPieceColors side);

		bool IsCheckMate(EPieceColors side);

	private:
		GameState _state;
	};
}
