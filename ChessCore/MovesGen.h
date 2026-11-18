#pragma once
#include <functional>
#include <map>
#include <optional>
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

		// The set-up the game began with (LI-01A): where castling's king and rooks start, and in which
		// form a castling is written (Chess960: the king onto its own rook).
		StartPosition Start;
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

		static bool IsUnderAttack(const BoardAttackMap& attackCache, const BoardPosition& positionPiece);

		// En passant as two physical moves: the pawn taken is first moved onto the target square.
		static bool AddComplementalMove(const Board& board, const Move& move, Move& complemental);

		// Castling (LI-01A). A side may still castle on that side while its king and that rook stand
		// on their start squares, the king has never moved and no move went from or to the rook's square.
		static bool HasCastlingRight(const GameState& gameState, PieceColors side, bool kingside);
		// That side's castling squares while it has the right and no square the king or that rook passes
		// or lands on (the two aside) is `taken`: empty squares for a legal move, our own pieces for a
		// premove. nullopt otherwise. Whether the king passes an attacked square is the caller's.
		static std::optional<CastlingSquares> CastlingPath(const GameState& gameState, PieceColors side, bool kingside,
		                                                   const std::function<bool(BoardPosition)>& taken);
		// The move is a castling of this game (standard: the king from its start two files over;
		// Chess960: the king onto its own rook): its squares in `squares`.
		static bool IsCastling(const GameState& gameState, const Move& move, CastlingSquares& squares);
		// A validated move played on the state's board, castling and en passant included (the history
		// is the caller's). `changed`: the squares it changed.
		static HistoryMove Play(const GameState& gameState, const Move& move, std::vector<BoardPosition>* changed = nullptr);

		static std::vector<PositionPiece> GetPositionsOf(const Board& board, PieceTypes type, PieceColors side);
	};
}