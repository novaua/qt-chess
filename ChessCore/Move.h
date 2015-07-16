#pragma once
#include "Board.h"

namespace Chess
{
	struct Move
	{
		BoardPosition From;

		BoardPosition To;

		bool Capturing;
	};

	struct PositionPiece
	{
		BoardPosition Position;
		Piece Piece;
	};

	struct HistoryMove
	{
		int Id;

		PositionPiece From;

		PositionPiece To;

		bool IsCapturingMove() const;
		Move ToMove() const;
	};

	struct MoveGeneration
	{
		static std::vector<Move> GenerateMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side);
	};
};