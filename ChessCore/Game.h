#pragma once
#include <memory>
#include <stack>
#include <functional>
#include <mutex>

#include "Board.h"
#include "Events.h"
#include "Move.h"
#include "HistoryPlayer.h"
#include "Queue.hpp"

namespace Chess
{
	typedef std::function<void(const EventBase &)> GameActionListener;
	typedef std::function<void(int index, const Piece &piece)> BoardChangesListener;
	typedef std::list<BoardChangesListener> BoardChangesListeners;
	typedef std::list<GameActionListener> GameActionListeners;

	class Game : public std::enable_shared_from_this < Game >
	{
		const char *SaveGameHeader = "com.chess.0.1";

		GameActionListeners _gameActionsListeners;
		BoardChangesListeners _boardChangesListeners;

		std::shared_ptr<Board> _board;
		MovesHistoryAptr _history;
		std::stack<Piece> _captured;
		bool _whiteFirst;
		MoveValidatorAptr _moveValidator;
		//std::vector<IMovesGeneratorAptr> _moveGenerators;

		MovesHistory _loadedHistory;
		std::mutex _lock;

		Queue <std::pair<bool, std::function<void()>>> _boardEventQueue;
		std::thread _boardEventHandler;

	public:
		Game();

		void RegisterGameActionsListeners(const GameActionListener & listener);
		void RegisterBoardChanged(const BoardChangesListener &listener);

		std::vector<Move> GetPossibleMoves(int index);
		std::vector<Move> GetAllowedMoves(int index);

		Piece GetPieceAt(int index);

		void Restart(bool whiteFirst = true);
		void EndGame();

		const MovesHistory &GetGameRecord() const;
		HistoryPlayerAptr MakePlayer();

		void Save(const std::string &path);
		void Load(const std::string &path);

		void DoMove(BoardPosition from, BoardPosition to);
		void UndoMove();

		bool IsWhiteMove();
		int GetMoveCount();

		~Game();
	private:
		void AssureMove(BoardPosition from, BoardPosition to);
		bool CanMoveFrom(BoardPosition from);
		void InitBoard();

		void NotifyBoardChangesListeners(std::vector<BoardPosition> indexes);
	};

	typedef std::shared_ptr<Game> GameAptr;
}
