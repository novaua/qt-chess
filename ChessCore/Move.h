#pragma once
#include "Board.h"

namespace Chess
{
	struct Move
	{
		BoardPosition From;

		BoardPosition To;

		bool Capturing;

		Piece PromotedTo;

		bool IsCheckmate;

		std::string ToString()const;
		static Move Parse(const std::string& strMove);
	};

	// Where a castling's king and rook go (LI-01A): the king to the g file and the rook to the f file
	// (O-O), or the king to c and the rook to d (O-O-O), whatever squares they start on.
	struct CastlingSquares
	{
		BoardPosition KingFrom;
		BoardPosition KingTo;
		BoardPosition RookFrom;
		BoardPosition RookTo;
		bool Kingside;

		static CastlingSquares Of(BoardPosition kingFrom, BoardPosition rookFrom);
	};

	struct PositionPiece
	{
		BoardPosition Position;

		Piece Piece;

		size_t GetHashCode() const;
	};

	struct HistoryMove
	{
		int Id;

		PositionPiece From;

		PositionPiece To;

		Piece PromotedTo;

		bool IsCapturingMove() const; // a piece of the other side was on the target square


		bool IsPawnPromotionMove() const;

		// Both forms: the king two files over (standard chess, e1g1) or onto its own rook (Chess960, e1h1).
		bool IsCastlingMove() const;
		CastlingSquares Castling() const; // a castling's squares (only for IsCastlingMove())

		bool IsEnPassantMove() const;

		// The piece this move took, or an empty piece. Also en passant when the target square was
		// empty (a bare Board move; Game records it as a normal capture).
		Piece CapturedPiece() const;

		Move ToMove() const;

		std::string ToUciString() const;
		static HistoryMove FromMove(const Move& move, const Piece& from, const Piece& to);
	};

	typedef std::vector<HistoryMove> MovesHistory;
	typedef std::shared_ptr<MovesHistory> MovesHistoryAptr;
}
