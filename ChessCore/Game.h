#pragma once
#include <memory>
#include <stack>
#include <functional>
#include <mutex>

#include "Board.h"
#include "Events.h"
#include "Move.h"
#include "HistoryPlayer.h"
#include "BoardPositionsCache.h"
#include "Check.h"

namespace Chess
{
	typedef std::function<void(const EventBase &)> GameActionListener;
	typedef std::function<void(int index, const Piece &piece)> BoardChangesListener;
	typedef std::function<void(const std::string &message)> LoggerCallback;

	typedef std::list<BoardChangesListener> BoardChangesListeners;
	typedef std::list<GameActionListener> GameActionListeners;

	class Game : public std::enable_shared_from_this < Game >
	{
		const char *SaveGameHeader = "com.chess.0.1";

		GameActionListeners _gameActionsListeners;
		BoardChangesListeners _boardChangesListeners;
		BoardPositionsCacheAptr _boardPositionsCacheAptr;

		std::shared_ptr<Board> _boardAptr;
		MovesHistoryAptr _historyAptr;
		MovesHistory _loadedHistory;

		GameState _gameState;

		std::stack<Piece> _captured;

		bool _whiteFirst;
		bool _checkMate;

		std::mutex _lock;
		LoggerCallback _logger;
		
	public:
		Game();

		void RegisterGameActionsListeners(const GameActionListener & listener);
		void RegisterBoardChanged(const BoardChangesListener &listener);
		
		void RegisterLogger(const LoggerCallback &loggerCallback);
		void Log(const std::string &message);

		std::vector<Move> GetPossibleMoves(int index);
		std::vector<Move> GetAllowedMoves(int index);

		Piece GetPieceAt(int index) const;

		void Restart(bool whiteFirst = true);
		void EndGame();

		const MovesHistory &GetGameRecord() const;
		HistoryPlayerAptr MakePlayer();

		void Save(const std::string &path);
		void Load(const std::string &path);

		void DoMove(BoardPosition from, BoardPosition to);
		void DoMove(const Move &move);
		void UndoMove();

		bool IsWhiteMove();
		int GetMoveCount();

		~Game();
	private:
		void AssureMove(BoardPosition from, BoardPosition to);
		bool CanMoveFrom(BoardPosition from);
		void NotifyFullBoardReinit();

		void NotifyBoardChangesListeners(std::vector<BoardPosition> indexes);
		void NotifyActionsListeners(const EventBase &event);
	};

	typedef std::shared_ptr<Game> GameAptr;
}
