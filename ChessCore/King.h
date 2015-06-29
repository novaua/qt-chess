#pragma once
#include "Piece.h"
namespace Chess
{
	class King: public Piece
	{
	public:
		King();
		~King();
		
		bool IsMoveValid(const Move &move, const Board &board);

		EPieceType GetType();
	};
}

