#include "stdafx.h"
#include "Board.h"
#include "ChessException.h"
#include "Move.h"

namespace Chess {

	Board::Board()
	{
		Initialize();
	}

	void Board::Initialize()
	{
		auto baseOffset = 0;
		_color = {};
		for (auto i = 0; i < 64; ++i)
		{
			if (i < 16)
			{
				_color.push_back(LIGHT);
			}
			else if (i < 16 + 32)
			{
				_color.push_back({ CEMPTY });
			}
			else
			{
				_color.push_back(DARK);
			}
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
	}

	Board::~Board()
	{
	}

	void Board::ValidateMove(const Move &move)
	{
		auto color = _color[move.From];
		auto found = false;
		for (auto m : MoveGeneration::GenerateMoves(*this, move.From, color))
		{
			if (m.From == move.From && m.To == move.To)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			throw ChessException("Invalid move!");
		}
	}

	Piece Board::At(BoardPosition position) const
	{
		return{ _piece[position], _color[position] };
	}

	void Board::Place(BoardPosition position, const Piece & piece)
	{
		_piece[position] = piece.Type;
		_color[position] = piece.Color;
	}

	HistoryMove Board::DoMove(const Move &move, bool force)
	{
		if (!force)
		{
			ValidateMove(move);
		}

		HistoryMove result = {};
		auto side = _color[move.From];
		assert(side != CEMPTY);

		if (_color[move.To] != CEMPTY)
		{
			assert(move.Capturing);
		}

		result.From = { move.From, At(move.From) };
		result.To = { move.To, At(move.To) };

		_piece[move.To] = _piece[move.From];
		_color[move.To] = _color[move.From];

		_piece[move.From] = EMPTY;
		_color[move.From] = CEMPTY;
		return result;
	}
}