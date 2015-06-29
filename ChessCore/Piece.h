#pragma once
#include <memory>

namespace Chess
{
	enum EPieceType
	{
		ept_pnil = 0, // empty
		ept_wpawn = 1,
		ept_bpawn = 2,
		ept_knight = 3,
		ept_bishop = 4,
		ept_rook = 5,
		ept_queen = 6,
		ept_king = 7,
	};

	struct Piece
	{
		virtual bool IsMoveValid(const Move &move, const Board &board);

		virtual EPieceType GetType();
	};

	typedef std::shared_ptr<Piece> PieceAptr;
};