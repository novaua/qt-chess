#pragma once
#include <map>
#include "Move.h"

#define BasicMovesGeneratorTypeName "Basic"
#define SituativeMovesGeneratorTypeName "Situative"

namespace Chess
{
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
		static void GetBoardAttackMap(const Board& board, BoardAttackMap& outCache, PieceColors side);
		static void GetBoardViktimsMap(const Board& board, BoardAttackMap& outCache, PieceColors side);

		static std::vector<Move> GenerateBasicMoves(const Board& board, BoardPosition pieceOffset, PieceColors side, bool attackingOnly = false);

		static std::vector<Move> GenerateAdvancedMoves(const GameState& gameState, BoardPosition pieceOffset, PieceColors side);

		static std::vector<Move> GenerateMoves(const GameState& gameState, BoardPosition pieceOffset, PieceColors side);
		static void ExcludeCheckMoves(const GameState& gameState, std::vector<Move>& moves, PieceColors side);

		static std::vector<Move> GenerateAllBasicMoves(const Board& board, PieceColors side);

		static void GeneratePawnMoves(std::vector<Move>& moves, const Board& board, BoardPosition pieceOffset, PieceColors side, bool attackingOnly = false);

		static bool IsValidCapturingMove(const Board& board, Move move, PieceColors side);

		static void Validate(const GameState& gameState, Move& move, PieceColors side);
		static void Validate(const GameState& gameState, const Move& move, PieceColors side);

		static bool IsEverMoved(const PositionPiece& positionPiece, const MovesHistory& history);
		static bool IsEverMoved(const Piece& piece, const MovesHistory& history);

		static bool IsUnderAttack(const BoardAttackMap& attackCache, const BoardPosition& positionPiece);

		//Translates En Passant and Castling moves to two physical move
		static bool AddComplementalMove(const Board& board, const Move& move, Move& complemental);

		static std::vector<PositionPiece> GetPositionsOf(const Board& board, PieceTypes type, PieceColors side);
	};
}