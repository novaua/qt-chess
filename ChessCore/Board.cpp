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
		_color = {};

		for (auto i = 0; i < 16; ++i)
		{
			_color.push_back(PieceColors::Light);
		}

		for (auto i = 0; i < 32; ++i)
		{
			_color.push_back(PieceColors::Empty);
		}

		for (auto i = 0; i < 16; ++i)
		{
			_color.push_back(PieceColors::Dark);
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

	Piece Board::At(int position) const
	{
		assert(a1 <= position && position < BpMax && "Invalid position!");
		return At((BoardPosition)position);
	}

	void Board::Place(BoardPosition position, const Piece& piece)
	{
		if (_piece[position] != piece.Type || _color[position] != piece.Color)
		{
			_piece[position] = piece.Type;
			_color[position] = piece.Color;
			OnBoardChanged(position, piece);
		}
	}

	HistoryMove Board::DoMove(const Move& move)
	{
		*_previousBoard = *this;

		HistoryMove result = {};

		auto fromPiece = move.PromotedTo.IsEmpty() ? At(move.From) : move.PromotedTo;

		auto toPiece = At(move.To);

		assert(!fromPiece.IsEmpty());

		if (!toPiece.IsEmpty())
		{
			assert(move.Capturing);
		}

		result.From = { move.From, At(move.From) };
		result.To = { move.To, At(move.To) };
		result.PromotedTo = move.PromotedTo;

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

	void Board::ForEachPiece(const std::function<void(BoardPosition)>& action, PieceColors colorFilter) const
	{
		for (auto i = 0u; i < _color.size(); ++i)
		{
			if (_color[i] == colorFilter)
			{
				action((BoardPosition)i);
			}
		}
	}

	std::string BoardPositionToString(BoardPosition value)
	{
		static std::map<int, std::string> intToStringMap;
		if (intToStringMap.empty())
		{
			auto a = 'a';
			for (auto i = 0; i < 8; ++i)
			{
				for (auto j = 0; j < 8; ++j)
				{
					char litera = a + j;
					std::stringstream ss;
					ss << litera << i + 1;
					intToStringMap[i * 8 + j] = ss.str();
				}
			}
		}

		return BpMax == value ? "-" : intToStringMap[value];
	}

	std::ostream& operator<<(std::ostream& out, BoardPosition value)
	{
		return out << BoardPositionToString(value);
	}

	std::vector<BoardPosition> BoardPositionFromString(const std::string& pos)
	{
		static std::map<std::string, int> strings;
		if (strings.empty())
		{
			for (int i = 0; i < BpMax; ++i)
			{
				std::stringstream ss;
				ss << static_cast<BoardPosition>(i);
				strings[ss.str()] = i;
			}
		}

		std::vector<BoardPosition> result;
		for (auto i = 0; i < pos.size(); i += 2)
		{
			auto candidate = pos.substr(i, 2);
			if (strings.find(candidate) != strings.end())
			{
				result.push_back(static_cast<BoardPosition>(strings[candidate]));
			}
		}

		return result;
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
