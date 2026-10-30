#pragma once
#include <functional>
#include <vector>
#include "Board.h"
#include "Move.h"

namespace Chess
{
	// A piece that went from one square to another. Its type changes for a promotion
	// (pawn -> piece) and for its takeback (piece -> pawn); From() == To() when only the type
	// changed (the player's promotion choice, made after the pawn reached the last rank).
	class PieceTravel
	{
	public:
		PieceTravel(const PositionPiece& from, const PositionPiece& to);

		BoardPosition From() const { return _from.Position; }
		BoardPosition To() const { return _to.Position; }
		const Piece& Before() const { return _from.Piece; }
		const Piece& After() const { return _to.Piece; }
		bool ChangesType() const { return !(_from.Piece == _to.Piece); }

	private:
		PositionPiece _from;
		PositionPiece _to;
	};

	// What changed between two positions, piece by piece. Found from the positions alone, so it
	// works for any change: a move, castling, en passant, promotion, a takeback, several moves at
	// once. A piece that left a square is matched with an equal piece that arrived elsewhere,
	// nearest first; a pawn that left the 7th rank (2nd for Black) with a piece of its colour that
	// arrived on the last rank beside it is a promotion (and the reverse a takeback of one).
	// What is left over was removed or added.
	class BoardDiff
	{
	public:
		BoardDiff(const Board& before, const Board& after);

		const std::vector<PieceTravel>& Moved() const { return _moved; }
		const std::vector<PositionPiece>& Removed() const { return _removed; } // captured (also en passant), or gone
		const std::vector<PositionPiece>& Added() const { return _added; }     // put back by a takeback, or new

		size_t Size() const { return _moved.size() + _removed.size() + _added.size(); }
		bool IsEmpty() const { return Size() == 0; }

	private:
		typedef std::function<bool(const PositionPiece& left, const PositionPiece& arrived)> SamePiece;

		// Moves pairs of a piece that left (_removed) and one that arrived (_added), where
		// samePiece allows it, nearest first, into _moved.
		void Pair(const SamePiece& samePiece);

		static bool IsSameKind(const PositionPiece& left, const PositionPiece& arrived);
		// The pawn became the piece: a step or a capture onto the last rank, or the type changing
		// in place on it.
		static bool IsPromotion(const PositionPiece& pawn, const PositionPiece& piece);
		static bool IsPromotionOrItsTakeback(const PositionPiece& left, const PositionPiece& arrived);

		static int FileOf(BoardPosition square) { return square % 8; }
		static int RankOf(BoardPosition square) { return square / 8; }
		static int Distance(BoardPosition a, BoardPosition b);

		std::vector<PieceTravel> _moved;
		std::vector<PositionPiece> _removed;
		std::vector<PositionPiece> _added;
	};
}
