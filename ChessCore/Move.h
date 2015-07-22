#pragma once
#include "Board.h"

#define BasicMovesGeneratorTypeName "Basic"
#define SituativeMovesGeneratorTypeName "Situative"

namespace Chess
{
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
		static std::vector<Move> GenerateMoves(const Board &board, BoardPosition pieceOffset, EPieceColors side);
	};

	class IMovesGenerator;
	typedef std::shared_ptr<IMovesGenerator> IMovesGeneratorAptr;

	class IMovesGenerator
	{
	public:

		virtual std::vector<Move> FindAllMoves(BoardPosition boardPosition) = 0;
	};

	class MovesGeneratorFactory
	{
		GameStateAptr _state;

	public:
		MovesGeneratorFactory(const GameStateAptr & state);

		IMovesGeneratorAptr CreateMovesGenerator(const std::string &type);
	};

	typedef std::shared_ptr<MovesGeneratorFactory> MovesGeneratorFactoryAptr;

	class BasicRulesMovesGenerator : public IMovesGenerator
	{
		BoardAptr _board;
	public:

		BasicRulesMovesGenerator(const BoardAptr &board);

		std::vector<Move> FindAllMoves(BoardPosition boardPosition);

		~BasicRulesMovesGenerator();
	};

	class SituativeMovesGenerator : public IMovesGenerator
	{
		GameStateAptr _state;

	public:

		SituativeMovesGenerator(const GameStateAptr  &state);

		std::vector<Move> FindAllMoves(BoardPosition boardPosition);

		~SituativeMovesGenerator();
	};

	class MoveValidator;
	typedef std::shared_ptr<MoveValidator> MoveValidatorAptr;

	class MoveValidator
	{
		std::vector<IMovesGeneratorAptr> _movesGenerators;
		BoardAptr _board;

	public:
		MoveValidator(const BoardAptr &board, const std::vector<IMovesGeneratorAptr> &movesGenerators);

		bool Validate(const Move &move);

		static MoveValidatorAptr Create(const BoardAptr &board, const MovesHistoryAptr &history);
	};
}
