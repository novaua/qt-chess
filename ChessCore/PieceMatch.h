#pragma once
#include <vector>
#include "Board.h"
#include "Move.h"

namespace Chess
{
	// A piece that went from one square to another. After differs from Before for a promotion
	// (pawn → piece) and its takeback (piece → pawn); From == To when only the type changed
	// (the player's promotion choice, after the pawn reached the last rank).
	struct PieceTravel
	{
		BoardPosition From;
		BoardPosition To;
		Piece Before;
		Piece After;
	};

	// What changed between two positions, piece by piece.
	struct BoardDiff
	{
		std::vector<PieceTravel> Moved;
		std::vector<PositionPiece> Removed; // captured (also en passant), or gone in a new position
		std::vector<PositionPiece> Added;   // a captured piece put back by a takeback, or new

		size_t Size() const { return Moved.size() + Removed.size() + Added.size(); }
		bool IsEmpty() const { return Size() == 0; }
	};

	// Which piece went where from `before` to `after`, found from the two positions alone, so it
	// works for any change: a move, castling, en passant, promotion, a takeback, several moves at
	// once. A piece that left a square is matched with an equal piece that arrived elsewhere,
	// nearest first; a pawn that left the 7th rank (2nd for Black) with a piece of its colour that
	// arrived on the last rank beside it is a promotion (and the reverse a takeback of one).
	// What is left over was removed or added.
	BoardDiff MatchPieces(const Board& before, const Board& after);
}
