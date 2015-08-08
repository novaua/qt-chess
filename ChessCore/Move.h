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

		size_t GetHashCode() const;
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

	class BoardPositionsCache;
	typedef std::shared_ptr<BoardPositionsCache> BoardPositionsCacheAptr;

	struct GameState
	{
		BoardAptr Board;

		MovesHistoryAptr History;

		BoardPositionsCacheAptr Cache;
	};

	typedef std::shared_ptr<GameState> GameStateAptr;

	// To -> {From1, FromN}
	typedef std::map<int, std::vector<PositionPiece>> BoardAttackMap;
	typedef std::shared_ptr<BoardAttackMap> BoardAttackMapAptr;

	struct MoveGeneration
	{
		static void GetBoardAttackMap(const Board &board, BoardAttackMap &outCache, EPieceColors side);
		static std::vector<Move> GenerateBasicMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, bool attackingOnly = false);

		static std::vector<Move> GenerateAdvancedMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history);

		static std::vector<Move> GenerateMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side, const MovesHistory &history);

		static std::vector<Move> GenerateAllBasicMoves(const Board &board, EPieceColors side);

		static void GeneratePawnMoves(std::vector<Move> &moves, const Board &board, BoardPosition pieceOffset, EPieceColors side, bool attackingOnly = false);

		static bool IsValidCapturingMove(const Board &board, Move move, EPieceColors side);

		static bool Validate(const Board &board, Move &move, EPieceColors side, const MovesHistory &history);

		static bool IsEverMoved(const PositionPiece &positionPiece, const MovesHistory &history);
		static bool IsEverMoved(const Piece &piece, const MovesHistory &history);

		static bool IsUnderAttack(BoardAttackMap & attackCache, const BoardPosition &positionPiece);

		//Translates En Passant and Castling moves to two physical move
		static bool AddComplementalMove(const Board &board, const Move &move, Move &complemental);
	};
}
