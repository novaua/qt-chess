#include "Move.h"

namespace Chess
{
	class GameChecks
	{
	public:
		GameChecks(const GameState &state);
		~GameChecks();

		bool IsInCheck(PieceColors side);

		bool IsCheckMate(PieceColors side);

		// Returns positive position on Pawn Promotion or -1 elsewhere.
		int IsInPawnPromotion(PieceColors side);

	private:
		GameState _state;
	};
}
