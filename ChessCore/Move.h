#pragma once
#include "Board.h"

#define BasicMovesGeneratorTypeName "Basic"
#define SituativeMovesGeneratorTypeName "Situative"

namespace Chess
{
	enum ComplexMoveType
	{
		CMEMPTY,
		CMCASTLING,
		CMENPASSANT,
	};

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

	typedef std::vector<HistoryMove> MovesHistory;
	typedef std::shared_ptr<MovesHistory> MovesHistoryAptr;

	struct GameState
	{
		BoardAptr Board;
		MovesHistoryAptr History;
	};

	typedef std::shared_ptr<GameState> GameStateAptr;

	struct MoveGeneration
	{
		static std::vector<Move> GenerateBasicMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side);

		static std::vector<Move> GenerateAdvancedMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history);

		static std::vector<Move> GenerateMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history);

		//static std::vector<BoardPosition> GetAttackersOf(const Board &board, BoardPosition pieceOffset, EPieceColors side);
		static bool IsValidCapturingMove(const Board &board, Move move, EPieceColors side);

		static bool Validate(const Board &board, Move &move, EPieceColors side, const MovesHistory &history);

		//Translates En Passant and Castling moves to two physical move
		static bool AddComplementalMove(const Board &board, const Move &move, Move &complemental);
	};
}