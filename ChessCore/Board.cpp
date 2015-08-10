#include "stdafx.h"
#include "Board.h"
#include "ChessException.h"
#include "Move.h"

namespace Chess {

	Board::Board()
	{
		for (auto i = 0; i < 64; ++i)
		{
			_color.push_back(CEMPTY);
			_piece.push_back(EMPTY);
		}
	}

	void Board::Initialize()
	{
		_color = {};

		for (auto i = 0; i < 16; ++i)
		{
			_color.push_back(LIGHT);
		}

		for (auto i = 0; i < 32; ++i)
		{
			_color.push_back(CEMPTY);
		}

		for (auto i = 0; i < 16; ++i)
		{
			_color.push_back(DARK);
		}

		_piece =
		{
			ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK,
			PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
			EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
			EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
			EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
			EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
			PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN, PAWN,
			ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK
		};

		_previousBoard = std::make_shared<Board>(*this);
	}

	Board::~Board()
	{
		if (BoardChanged)
		{
			BoardChanged = {};
		}
	}

	Piece Board::At(BoardPosition position) const
	{
		return{ _piece[position], _color[position] };
	}

	void Board::Place(BoardPosition position, const Piece & piece)
	{
		if (_piece[position] != piece.Type || _color[position] != piece.Color)
		{
			_piece[position] = piece.Type;
			_color[position] = piece.Color;
			OnBoardChanged(position, piece);
		}
	}

	HistoryMove Board::DoMove(const Move &move)
	{
		*_previousBoard = *this;

		HistoryMove result = {};

		auto fromPiece = At(move.From);
		auto toPiece = At(move.To);

		assert(!fromPiece.IsEmpty());

		if (!toPiece.IsEmpty())
		{
			assert(move.Capturing);
		}

		result.From = { move.From, At(move.From) };
		result.To = { move.To, At(move.To) };

		Place(move.From, {});
		Place(move.To, fromPiece);

		return result;
	}

	void Board::UndoLastMove()
	{
		*this = *_previousBoard;
	}

	void Board::OnBoardChanged(BoardPosition pos, Piece newValue)
	{
		if (BoardChanged)
		{
			BoardChanged(pos, newValue);
		}
	}

	void Board::ForEachPiece(const std::function<void(BoardPosition)> &action, EPieceColors color) const
	{
        for (auto i = 0u; i < _color.size(); ++i)
		{
			if (_color[i] == color)
			{
				action((BoardPosition)i);
			}
		}
	}

	//position hash code using Zobrist Hashing
	size_t Board::GetHashCode() const
	{
		size_t hash = 0;
        for (auto i = 0u; i < _color.size(); ++i)
		{
			PositionPiece pp = { (BoardPosition)i, At((BoardPosition)i) };
			hash ^= pp.GetHashCode();
		}

		return hash;
	}
}
