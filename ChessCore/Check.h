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

		// Returns positive position on Pawn Promotion or -1 elsewhere.
		int IsInPawnPromotion(EPieceColors side);

	private:
		GameState _state;
	};
}
