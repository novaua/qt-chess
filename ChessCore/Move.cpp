#include "stdafx.h"
#include "Move.h"
#include "ChessException.h"
#include "BoardPositionsCache.h"
#include "Check.h"
//#include <strstream>

using namespace Chess;

bool HistoryMove::IsCapturingMove() const
{
	return (To.Piece.Type != EMPTY);
}

bool HistoryMove::IsPawnPromotionMove() const
{
	return PromotedTo.Type != EMPTY;
}

Move HistoryMove::ToMove() const
{
	return{ From.Position, To.Position, false, PromotedTo };
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

void MoveGeneration::GeneratePawnMoves(std::vector<Move> &moves, const Board &board, BoardPosition pieceOffset, PieceColors side, bool attackingOnly)
{
	// pawn moves
	auto p = board.piece()[pieceOffset];
	if (p != PAWN)
	{
		return;
	}

	auto colorDirection = board.At(pieceOffset).Color == PieceColors::Dark ? -1 : 1;
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
				if (board.color()[n] != PieceColors::Empty) {
					if (board.color()[n] != side)
					{
						moves.push_back({ pieceOffset, (BoardPosition)n, true }); /* capture from i to n */
					}
				}
				else if (attackingOnly)
				{
					moves.push_back({ pieceOffset, (BoardPosition)n, false }); /* potential attacking move */
				}

				break;
			}

			if (board.color()[n] == PieceColors::Empty && !attackingOnly)
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

std::vector<Move> MoveGeneration::GenerateBasicMoves(const Board &board, BoardPosition pieceOffset, PieceColors side, bool attackingOnly)
{
	std::vector<Move> moves;
	auto p = board.piece()[pieceOffset];
	if (p != PAWN) { /* piece or pawn */
		for (auto j = 0; j < offsets[p]; ++j) { /* for all knight or ray directions */
			for (int n = pieceOffset;;) { /* starting with from square */

				n = mailbox[mailbox64[n] + offset[p][j]]; /* next square along the ray j */

				if (n == -1)
					break; /* outside board */

				if (board.color()[n] != PieceColors::Empty) {
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
		MoveGeneration::GeneratePawnMoves(moves, board, pieceOffset, side, attackingOnly);
	}

	return moves;
}

bool MoveGeneration::IsValidCapturingMove(const Board &board, Move move, PieceColors side)
{
	auto result = false;
	for (auto locMove : GenerateBasicMoves(board, move.From, side, true))
	{
		if (locMove.To == move.To)
		{
			result = true;
			break;
		}
	}

	return result;
}

std::vector<Move> MoveGeneration::GenerateAdvancedMoves(const GameState &gameState, BoardPosition pieceOffset, PieceColors side)
{
	if (gameState.History->empty())
	{
		return{};
	}

	std::vector<Move> result;

	const Board &board = *gameState.Board;
	auto &history = *gameState.History;

	auto oppositeSide = OppositeSideOf(side);
	auto oppositeMoveDirection = oppositeSide == PieceColors::Dark ? -1 : 1;
	auto imThePiece = board.At(pieceOffset);

	auto attackCache = gameState.Cache->GetAttackMap(gameState.Board, oppositeSide);

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
	else if (imThePiece.Type == KING && !IsEverMoved(imThePiece, history) && !IsUnderAttack(*attackCache, pieceOffset))
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
				if (board.color()[emptyOffset] != PieceColors::Empty)
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
						for (auto mv : GenerateAllBasicMoves(board, oppositeSide))
						{
							allMovesToSet.insert(mv.To);
						}
					}

					auto kingPathIsUnderAttack = false;
					int notUnderAtack[] = { pieceOffset + stepsDirection[i], pieceOffset + 2 * stepsDirection[i] };
					for (auto p : notUnderAtack)
					{
						if (allMovesToSet.find(BoardPosition(p)) != allMovesToSet.end())
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

std::vector<Move> MoveGeneration::GenerateMoves(const GameState &gameState, BoardPosition pieceOffset, PieceColors side)
{
	auto moves = MoveGeneration::GenerateBasicMoves(*gameState.Board, pieceOffset, side);
	auto adMoves = MoveGeneration::GenerateAdvancedMoves(gameState, pieceOffset, side);

	std::copy(adMoves.begin(), adMoves.end(), std::back_inserter(moves));
	return moves;
}

void MoveGeneration::ExcludeCheckMoves(const GameState &gameState, std::vector<Move> &moves, PieceColors side)
{
	auto simBoard = std::make_shared<Board>(*gameState.Board);
	simBoard->BoardChanged = nullptr;

	//this preparations are quite ugly. Consider to re-factor!
	GameState simState = gameState;
	simState.Board = simBoard;

	GameChecks check(simState);
	std::vector<Move> newMoves;
	bool checkDetected = false;

	for (const auto& move : moves)
	{
		simBoard->DoMove(move);
		if (!check.IsInCheck(side))
		{
			newMoves.push_back(move);
		}
		else
		{
			checkDetected = true;
		}

		simBoard->UndoLastMove();
	}

	if (checkDetected)
	{
		newMoves.swap(moves);
	}
}

void MoveGeneration::Validate(const GameState &gameState, Move &move, PieceColors side)
{
	auto found = false;
	for (auto mv : MoveGeneration::GenerateMoves(gameState, move.From, side))
	{
		if (mv.From == move.From && mv.To == move.To)
		{
			found = true;
			move.Capturing = mv.Capturing;
			break;
		}
	}

	if (!found)
	{
		throw ChessException("Invalid move!");
	}
}

void MoveGeneration::Validate(const GameState &gameState, const Move &move, PieceColors side)
{
	auto move1 = move;
	Validate(gameState, move1, side);
}

bool MoveGeneration::AddComplementalMove(const Board &board, const Move &move, Move &complemental)
{
	auto pieceMovesFrom = board.At(move.From);
	auto pieceMoveTo = board.At(move.To);
	auto otherSideMoveDirection = pieceMovesFrom.Color == PieceColors::Light ? -1 : 1;

	if (move.Capturing && pieceMovesFrom.Type == PAWN && pieceMoveTo.Color == PieceColors::Empty)
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
	for (auto move : history)
	{
		if (move.From.Piece == positionPiece.Piece && move.From.Position == positionPiece.Position)
		{
			return true;
		}
	}

	return false;
}

bool MoveGeneration::IsEverMoved(const Piece &piece, const MovesHistory &history)
{
	for (auto move : history)
	{
		if (move.From.Piece == piece)
		{
			return true;
		}
	}

	return false;
}

std::vector<Move> MoveGeneration::GenerateAllBasicMoves(const Board &board, PieceColors side)
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

void MoveGeneration::GetBoardAttackMap(const Board &board, BoardAttackMap &outCache, PieceColors side)
{
	board.ForEachPiece([&](BoardPosition moveFrom)
	{
		auto moves = GenerateBasicMoves(board, moveFrom, side, true);
		for (auto move : moves)
		{
			outCache[move.To].push_back({ move.From, board.At(move.From) });
		}
	}, side);
}

void MoveGeneration::GetBoardViktimsMap(const Board &board, BoardAttackMap &outCache, PieceColors side)
{
	board.ForEachPiece([&](BoardPosition moveFrom)
	{
		auto moves = GenerateBasicMoves(board, moveFrom, side, true);
		for (auto move : moves)
		{
			outCache[move.From].push_back({ move.To, board.At(move.To) });
		}
	}, side);
}

bool MoveGeneration::IsUnderAttack(const BoardAttackMap & attackCache, const BoardPosition &position)
{
	return attackCache.find(position) != attackCache.end();
}

std::vector<PositionPiece> MoveGeneration::GetPositionsOf(const Board &board, PieceTypes type, PieceColors side)
{
	std::vector<PositionPiece> resultList;
	unsigned int maxCount = GetPiceCount(type);
	for (auto i = 0; i < BpMax; i++)
	{
		auto p = board.At(BoardPosition(i));
		if (p.Type == type && p.Color == side)
		{
			resultList.push_back({ BoardPosition(i), p });
			if (resultList.size() == maxCount)
			{
				break;
			}
		}
	}

	return resultList;
}

size_t PositionPiece::GetHashCode() const
{
	static auto randomVector = MakeRandomVector(BpMax * UniquePiecesCount * 2);
	return  Piece.IsEmpty() ? 0 : randomVector[Piece.GetHashCode() - 1 + Position * UniquePiecesCount * 2];
}

const std::string MoveBegin("Move(");
const std::string MoveDelim(",");
const std::string MoveEnd(")");

std::string Move::ToString()const
{
	auto delim = MoveDelim;

	//from, to, capturing, piece
	std::stringstream str;
	str << MoveBegin << From << delim << To << delim << (Capturing ? "true" : "false") << delim << PromotedTo.ToString() << MoveEnd << '\0';
	return str.str();
}

Move Move::Empty;

Move Move::Parse(const std::string &strMove)
{
	Move result = {};
	size_t parsePtr = 0U;
	if (strMove.substr(0, MoveBegin.size()) == MoveBegin)
	{
		int member = 0;
		parsePtr += MoveBegin.size();
		auto delimPtr = parsePtr;

		while (parsePtr < strMove.size())
		{
			while (MoveDelim[0] != strMove[delimPtr] && strMove[delimPtr] != '\0' && strMove[delimPtr] != MoveEnd[0])
			{
				++delimPtr;
			}

			auto tokenSize = delimPtr - parsePtr;
			std::string token = strMove.substr(parsePtr, tokenSize);
			parsePtr += tokenSize + 1;

			if (member == 0)
			{
				result.From = (BoardPosition)atoi(token.c_str());
				member++;
			}
			else if (member == 1)
			{
				result.To = (BoardPosition)atoi(token.c_str());
				member++;
			}
			else if (member == 2)
			{
				result.Capturing = token == "true";
				member++;
			}
			else if (member == 3)
			{
				result.PromotedTo = Piece::Parse(token);
				member++;
			}

			if (strMove[delimPtr] == MoveEnd[0])
				break;

			++delimPtr;
		}
	}

	return result;
}

Move Chess::FromBoardPositions(const std::vector<BoardPosition> &positions)
{
	return{ positions[0], positions[1] };
}