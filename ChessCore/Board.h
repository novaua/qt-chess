#pragma once
#include <vector>
#include <ostream>
#include "Piece.h"

namespace Chess {

	enum BoardPosition {
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8,
		BpMax
	};

	struct Move;
	struct HistoryMove;
	struct PositionPiece;

	class Board;
	typedef std::shared_ptr<Board> BoardAptr;

	class Board
	{
		std::vector<EPieceColors> _color;  /* EMPTY, LIGHT, DARK */
		std::vector<EPieceTypes> _piece;  /* PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, or EMPTY */
		BoardAptr _previousBoard;

	public:

		std::function<void(BoardPosition pos, Piece newValue)> BoardChanged;

		Board();
		~Board();

		void Initialize();

		const std::vector<EPieceColors> &color() const
		{
			return _color;
		}

		const std::vector<EPieceTypes> &piece() const
		{
			return _piece;
		}

		Piece At(BoardPosition position) const;
		void Place(BoardPosition position, const Piece & piece);

		void ForEachPiece(const std::function<void(BoardPosition)> &action, EPieceColors color) const;

		// Does non-empty Piece move without chess basic rules validation
		HistoryMove DoMove(const Move &move);
		void UndoLastMove();

		//position hash code
		size_t GetHashCode() const;

	private:
		void OnBoardChanged(BoardPosition pos, Piece newValue);
	};
}
