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

		std::string ToString()const;
		static Move Parse(const std::string& strMove);
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

		bool IsCapturingMove() const;

		bool IsPawnPromotionMove() const;

		Move ToMove() const;

		std::string ToUciString() const;
	};

	typedef std::vector<HistoryMove> MovesHistory;
	typedef std::shared_ptr<MovesHistory> MovesHistoryAptr;
}
