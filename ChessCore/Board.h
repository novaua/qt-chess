#pragma once
#include <vector>
#include <ostream>
#include <functional>
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

	std::ostream& operator<<(std::ostream& out, BoardPosition value);
	std::vector<BoardPosition> BoardPositionFromString(const std::string& pos);
	std::string BoardPositionToString(BoardPosition value);

	class Board
	{
		std::vector<PieceColors> _color;  /* Empty, Light, Dark */
		std::vector<PieceTypes> _piece;  /* PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, or Empty */
		BoardAptr _previousBoard;

	public:

		std::function<void(BoardPosition pos, Piece newValue)> BoardChanged;

		Board();
		~Board();

		void Initialize();

		const std::vector<PieceColors> &color() const
		{
			return _color;
		}

		const std::vector<PieceTypes> &piece() const
		{
			return _piece;
		}

		Piece At(BoardPosition position) const;
		Piece At(int position) const;

		void Place(BoardPosition position, const Piece & piece);

		void ForEachPiece(const std::function<void(BoardPosition)> &action, PieceColors color) const;

		// Does non-empty Piece move without chess basic rules validation. However it does capturing check.
		HistoryMove DoMove(const Move &move);
		void UndoLastMove();

		//position hash code
		size_t GetHashCode() const;

	private:
		void OnBoardChanged(BoardPosition pos, Piece newValue);
	};
}
