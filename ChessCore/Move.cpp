#include "stdafx.h"
#include "Move.h"
#include "ChessException.h"

using namespace Chess;

bool HistoryMove::IsCapturingMove() const
{
	return (To.Piece.Type != EMPTY);
}

Move HistoryMove::ToMove() const
{
	return{ From.Position, To.Position };
}

namespace {
	// Standard approach to decide where to move.
	// This code mainly taken from http://chessprogramming.wikispaces.com/10x12+Board#OffsetMG
	// Style and comments were mostly preserved.
	bool slide[] = { false, false, true, true, true, false, false };

	int offsets[] = { 0, 8, 4, 4, 8, 8, 3 }; /* knight or ray directions */

	int offset[][8] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ -21, -19, -12, -8, 8, 12, 19, 21 }, /* KNIGHT */
		{ -11, -9, 9, 11, 0, 0, 0, 0 }, /* BISHOP */
		{ -10, -1, 1, 10, 0, 0, 0, 0 }, /* ROOK */
		{ -11, -10, -9, -1, 1, 9, 10, 11 }, /* QUEEN */
		{ -11, -10, -9, -1, 1, 9, 10, 11 },  /* KING */
		{ 9, 11, 10, 0, 0, 0, 0, 0 } /* PAWN */
	};

	int pawn_capturing[] = { 1, 1, 0, 0, 0, 0, 0 }; /* 1 for mandatory capturing move */

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

std::vector<Move> MoveGeneration::GenerateBasicMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side)
{
	std::vector<Move> moves;
	auto p = board.piece()[pieceOffset];
	if (p != PAWN) { /* piece or pawn */
		for (auto j = 0; j < offsets[p]; ++j) { /* for all knight or ray directions */
			for (int n = pieceOffset;;) { /* starting with from square */

				n = mailbox[mailbox64[n] + offset[p][j]]; /* next square along the ray j */

				if (n == -1)
					break; /* outside board */

				if (board.color()[n] != CEMPTY) {
					if (board.color()[n] != side)
						moves.push_back({ pieceOffset, (BoardPosition)n, true }); /* capture from i to n */
					break;
				}

				moves.push_back({ pieceOffset, (BoardPosition)n, false }); /* quiet move from i to n */
				if (!slide[p]) break; /* next direction */
			}
		}
	}
	else
	{
		// pawn moves
		auto colorDirection = board.At(pieceOffset).Color == DARK ? -1 : 1;
		auto rank = colorDirection > 0 ? (pieceOffset / 8) : 7 - (pieceOffset / 8);
		auto forwardMaxMoveLength = rank == 1 ? 2 : 1;

		for (auto j = 0; j < offsets[p]; ++j)
		{
			for (int n = pieceOffset;;)
			{
				n = mailbox[mailbox64[n] + colorDirection * offset[p][j]];
				if (n == -1)
				{
					break;
				}

				if (pawn_capturing[j])
				{
					if (board.color()[n] != CEMPTY) {
						if (board.color()[n] != side)
						{
							moves.push_back({ pieceOffset, (BoardPosition)n, true }); /* capture from i to n */
						}
					}

					break;
				}

				if (board.color()[n] == CEMPTY)
				{
					moves.push_back({ pieceOffset, (BoardPosition)n, false }); /* quiet move from i to n */
					--forwardMaxMoveLength;
				}
				else
				{
					break;
				}

				if (forwardMaxMoveLength <= 0)
				{
					break;
				}
			}
		}
	}

	return moves;
}

bool MoveGeneration::IsValidCapturingMove(const Board &board, Move move, EPieceColors side)
{
	auto result = false;
	auto myBoard = board;
	auto me = board.At(move.From);
	auto oppositeSide = side == DARK ? LIGHT : DARK;

	//placing additional fake pawn to find all moves for this PAWN
	if (me.Type == PAWN && board.color()[move.To] == CEMPTY)
	{
		myBoard.Place(move.To, { PAWN, oppositeSide });
	}

	for each(auto locMove in GenerateBasicMoves(myBoard, move.From, side))
	{
		if (locMove.To == move.To)
		{
			result = true;
			break;
		}
	}

	return result;
}

std::vector<Move> MoveGeneration::GenerateAdvancedMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history)
{
	if (history.empty())
	{
		return{};
	}

	std::vector<Move> result;

	auto oppositeSide = side == DARK ? LIGHT : DARK;
	auto oppositeMoveDirection = oppositeSide == DARK ? -1 : 1;
	auto imThePiece = board.At(pieceOffset);

	// check if the last opposite side move was made by Peasant
	auto lastMove = *history.crbegin();
	if (imThePiece.Type == PAWN &&lastMove.From.Piece.Type == PAWN && lastMove.From.Piece.Color == oppositeSide
		&& lastMove.To.Position - lastMove.From.Position == 16 * oppositeMoveDirection)
	{
		auto peaceOneRankMove = lastMove.From.Position + 8 * oppositeMoveDirection;
		assert(0 <= peaceOneRankMove && peaceOneRankMove < 64 && "Out of the board!");

		// check if the previous rank field is under attack
		if (IsValidCapturingMove(board, { pieceOffset, (BoardPosition)peaceOneRankMove }, side))
		{
			result.push_back({ pieceOffset, (BoardPosition)peaceOneRankMove, true });
		}
	}
	else if (imThePiece.Type == KING && !IsEverMoved(imThePiece, history)
		/* ToDo: &&!IsInCheck(imThePiece) */)
	{
		static const int rookOffsets[] = { -4, 3 };
		static const int stepsDirection[] = { -1, 1 };

		static const std::function<bool(int, int)> lessCheck[] =
		{
			{ [](int l, int r) {return r < l; } },
			{ std::less<int>() }
		};

		std::set<BoardPosition> allMovesToSet;
		for (int i = 0; i < 2; ++i)
		{
			auto boardOpened = true;
			for (int p = stepsDirection[i]; lessCheck[i](p, rookOffsets[i]); p += stepsDirection[i])
			{
				auto emptyOffset = pieceOffset + p;
				if (board.color()[emptyOffset] != CEMPTY)
				{
					boardOpened = false;
				}
			}

			if (boardOpened)
			{
				auto pos = BoardPosition(pieceOffset + rookOffsets[i]);
				auto rook = board.At(pos);
				if (!IsEverMoved({ pos, rook }, history))
				{
					if (allMovesToSet.empty())
					{
						// ToDo: This does not include pawn moves
						for each(auto mv in GenerateAllBasicMoves(board, oppositeSide, history))
						{
							allMovesToSet.insert(mv.To);
						}
					}

					auto kingPathIsUnderAttack = false;
					int notUnderAtack[] = { pieceOffset + stepsDirection[i], pieceOffset + 2 * stepsDirection[i] };
					for each (BoardPosition p in notUnderAtack)
					{
						if (allMovesToSet.find(p) != allMovesToSet.end())
						{
							kingPathIsUnderAttack = true;
							break;
						}
					}

					if (!kingPathIsUnderAttack)
					{
						result.push_back({ pieceOffset, BoardPosition(pieceOffset + 2 * stepsDirection[i]) });
					}
				}
			}
		}
	}

	return result;
}

std::vector<Move> MoveGeneration::GenerateMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history)
{
	auto moves = MoveGeneration::GenerateBasicMoves(board, pieceOffset, side);

	auto adMoves = MoveGeneration::GenerateAdvancedMoves(board, pieceOffset, side, history);
	std::copy(adMoves.begin(), adMoves.end(), std::back_inserter(moves));

	return moves;
}

bool MoveGeneration::Validate(const Board &board, Move &move, EPieceColors side, const MovesHistory &history)
{
	auto found = false;
	for (auto mv : MoveGeneration::GenerateMoves(board, move.From, side, history))
	{
		if (mv.From == move.From && mv.To == move.To)
		{
			found = true;
			move = mv;
			break;
		}
	}

	if (!found)
	{
		throw ChessException("Invalid move!");
	}

	return found;
}

bool MoveGeneration::AddComplementalMove(const Board &board, const Move &move, Move &complemental)
{
	auto pieceMovesFrom = board.At(move.From);
	auto pieceMoveTo = board.At(move.To);
	auto otherSideMoveDirection = pieceMovesFrom.Color == LIGHT ? -1 : 1;

	if (move.Capturing && pieceMovesFrom.Type == PAWN && pieceMoveTo.Color == CEMPTY)
	{
		//It's expected that the move was validated before. Here we just translate it to physical moves.
		// En Passant detected
		auto pawnRemovePosition = (BoardPosition)((int)move.To + otherSideMoveDirection * 8);
		auto otherSiedePawn = board.At(pawnRemovePosition);

		assert(otherSiedePawn.Type == PAWN && otherSiedePawn.Color != pieceMovesFrom.Color && "Other side Pawn is expected here!");

		//returning the other side pawn one step back
		complemental = { pawnRemovePosition, move.To, false };
		return true;
	}

	auto dt = move.From - move.To;
	auto dtByModule = dt < 0 ? dt * -1 : dt;

	if (!move.Capturing && pieceMovesFrom.Type == KING && dtByModule == 2)
	{
		// Castling
		static int  rookMovesTable[] = { -4, 3, 3, -2 };
		int ptr = (dt < 0) ? 2 : 0;

		auto rookPos = (BoardPosition)(move.From + rookMovesTable[ptr++]);
		auto rook = board.At(rookPos);
		assert(rook.Type == ROOK && rook.Color == pieceMovesFrom.Color && "Rook and King are expected of the same side");

		complemental = { rookPos, (BoardPosition)(rookPos + rookMovesTable[ptr]) };
		return true;
	}

	return false;
}

bool MoveGeneration::IsEverMoved(const PositionPiece &positionPiece, const MovesHistory &history)
{
	for each (auto move in history)
	{
		if (move.From.Piece == positionPiece.Piece && (move.From.Position == positionPiece.Position))
		{
			return true;
		}
	}

	return false;
}

bool MoveGeneration::IsEverMoved(const Piece &piece, const MovesHistory &history)
{
	for each (auto move in history)
	{
		if (move.From.Piece == piece)
		{
			return true;
		}
	}

	return false;
}

std::vector<Move> MoveGeneration::GenerateAllBasicMoves(const Board &board, EPieceColors side, const MovesHistory &history)
{
	std::vector<Move> moves;
	board.ForEachPiece(std::function<void(BoardPosition)>([&](BoardPosition pos)
	{
		auto localMoves = MoveGeneration::GenerateBasicMoves(board, pos, side);
		moves.insert(moves.end(),
			std::make_move_iterator(localMoves.begin()),
			std::make_move_iterator(localMoves.end()));
	}), side);

	return moves;
}