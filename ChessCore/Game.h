#pragma once
#include <memory>
#include <stack>
#include <functional>

#include "Board.h"
#include "Events.h"
#include "Move.h"

namespace Chess
{
	typedef std::function<void(const EventBase &)> GameActionListener;
	typedef std::function<void(int index, const Piece &piece)> BoardChangesListener;
	typedef std::list<BoardChangesListener> BoardChangesListeners;

	typedef std::vector<HistoryMove> GameHistory;

	class Game
	{
		const int AutoPlayMoveWaitSeconds = 3;
		const char *SaveGameHeader = "com.chess.0.1";

		std::list<GameActionListener> _gameActionsListeners;
		BoardChangesListeners _boardChangesListeners;

		std::unique_ptr<Board> _board;
		std::vector<HistoryMove> _history;
		std::stack<Piece> _captured;
		bool _whiteFirst;

		std::vector<HistoryMove> _loadedHistory;

	public:
		Game();

		void RegisterGameActionsListeners(const GameActionListener & listener);
		void RegisterBoardChanged(const BoardChangesListener &listener);

		std::vector<Move> GetPossibleMoves(int index);
		std::vector<Move> GetAllowedMoves(int index);

		Piece GetPieceAt(int index);

		void Restart(bool whiteFirst = true);

		const GameHistory &GetGameRecord() const;
		void Play(const GameHistory &gameHistory);

		void Save(const std::string &path);
		void Load(const std::string &path);

		void DoMove(BoardPosition from, BoardPosition to);
		void UndoMove();

		bool IsWhiteMove();
		int GetMoveCount() const;

		~Game();
	private:
		void AssureMove(BoardPosition from, BoardPosition to);
		bool CanMoveFrom(BoardPosition from);

		void NotifyBoardChangesListeners(std::vector<BoardPosition> indexes);
	};
}
