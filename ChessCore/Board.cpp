#include "stdafx.h"
#include "Board.h"
#include "ChessException.h"

namespace Chess {

	Board::Board(bool whiteFirst)
	{
		_whiteFirst = whiteFirst;

		auto baseOffset = 0;
		for (auto i = 0; i < 64; ++i)
		{
			if (i < 16)
			{
				_color.push_back({ whiteFirst ? LIGHT : DARK });
			}
			else if (i < 32)
			{
				_color.push_back({ EC_EMPTY });
			}
			else
			{
				_color.push_back({ !whiteFirst ? LIGHT : DARK });
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

	Move Board::DoMove(const Move &move, bool force)
	{
		if (!force)
		{
			ValidateMove(move);
		}

		Move result = {};
		auto side = _color[move.From];
		assert(side != EC_EMPTY);

		if (_color[move.To] != EC_EMPTY)
		{
			assert(move.Capturing);
			result.Capturing = false;
			result.Captured = { _piece[move.To], _color[move.To] };
		}

		result.From = move.To;
		result.To = move.From;

		_piece[move.To] = _piece[move.From];
		_color[move.To] = _color[move.From];

		_piece[move.From] = EMPTY;
		_color[move.From] = EC_EMPTY;
		return result;
	}

	namespace {
		// Standard approach to decide where to move.
		// This code mainly taken from http://chessprogramming.wikispaces.com/10x12+Board#OffsetMG
		// Style and comments were preserved.
		bool slide[6] = { false, false, true, true, true, false };

		int offsets[6] = { 0, 8, 4, 4, 8, 8 }; /* knight or ray directions */

		int offset[6][8] = {
				{ 0, 0, 0, 0, 0, 0, 0, 0 },
				{ -21, -19, -12, -8, 8, 12, 19, 21 }, /* KNIGHT */
				{ -11, -9, 9, 11, 0, 0, 0, 0 }, /* BISHOP */
				{ -10, -1, 1, 10, 0, 0, 0, 0 }, /* ROOK */
				{ -11, -10, -9, -1, 1, 9, 10, 11 }, /* QUEEN */
				{ -11, -10, -9, -1, 1, 9, 10, 11 }  /* KING */
		};

		/* Now we have the mailbox array, so called because it looks like a
		mailbox, at least according to Bob Hyatt. This is useful when we
		need to figure out what pieces can go where. Let's say we have a
		rook on square a4 (32) and we want to know if it can move one
		square to the left. We subtract 1, and we get 31 (h5). The rook
		obviously can't move to h5, but we don't know that without doing
		a lot of annoying work. Sooooo, what we do is figure out a4's
		mailbox number, which is 61. Then we subtract 1 from 61 (60) and
		see what mailbox[60] is. In this case, it's -1, so it's out of
		bounds and we can forget it. You can see how mailbox[] is used
		in attack() in board.c. */

		int mailbox[120] = {
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, 0, 1, 2, 3, 4, 5, 6, 7, -1,
			-1, 8, 9, 10, 11, 12, 13, 14, 15, -1,
			-1, 16, 17, 18, 19, 20, 21, 22, 23, -1,
			-1, 24, 25, 26, 27, 28, 29, 30, 31, -1,
			-1, 32, 33, 34, 35, 36, 37, 38, 39, -1,
			-1, 40, 41, 42, 43, 44, 45, 46, 47, -1,
			-1, 48, 49, 50, 51, 52, 53, 54, 55, -1,
			-1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1, -1, -1, -1, -1, -1, -1
		};

		int mailbox64[64] = {
			21, 22, 23, 24, 25, 26, 27, 28,
			31, 32, 33, 34, 35, 36, 37, 38,
			41, 42, 43, 44, 45, 46, 47, 48,
			51, 52, 53, 54, 55, 56, 57, 58,
			61, 62, 63, 64, 65, 66, 67, 68,
			71, 72, 73, 74, 75, 76, 77, 78,
			81, 82, 83, 84, 85, 86, 87, 88,
			91, 92, 93, 94, 95, 96, 97, 98
		};
	}

	std::vector<Move> MoveGeneration::GenerateMoves(const Board &board, int pieceOffset, EPieceColors side)
	{
		std::vector<Move> moves;
		auto p = board.piece()[pieceOffset];
		if (p != PAWN) { /* piece or pawn */
			for (auto j = 0; j < offsets[p]; ++j) { /* for all knight or ray directions */
				for (auto n = pieceOffset;;) { /* starting with from square */

					n = mailbox[mailbox64[n] + offset[p][j]]; /* next square along the ray j */

					if (n == -1)
						break; /* outside board */

					if (board.color()[n] != EC_EMPTY) {
						if (board.color()[n] == !side)
							moves.push_back({ pieceOffset, n, true }); /* capture from i to n */
						break;
					}

					moves.push_back({ pieceOffset, n, false }); /* quiet move from i to n */
					if (!slide[p]) break; /* next direction */
				}
			}
		}
		else
		{
			/* pawn moves */
			auto direction = side == LIGHT && board.AreWhiteFirst() ? 1 : -1;

			for (auto toGo : { pieceOffset + 8 * direction, pieceOffset + 16 * direction })
			{
				if (board.color()[toGo] == EC_EMPTY)
				{
					moves.push_back({ pieceOffset, toGo, false });
				}
			}

			for (auto toStrike : { pieceOffset + 7 * direction, pieceOffset + 9 * direction })
			{
				if (board.color()[toStrike] != EC_EMPTY && board.color()[toStrike] != side)
				{
					moves.push_back({ pieceOffset, toStrike, true });
				}
			}
		}

		return moves;
	}

	void Move::Serialize(std::ostream &ostream) const
	{
		ostream.write((char*)this, sizeof(*this));
	}

	Move Move::Deserialize(std::istream &istream)
	{
		Move out;
		istream.read((char*)&out, sizeof(out));
		return out;
	}
}