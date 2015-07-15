#pragma once
#include "Board.h"
#include "Events.h"
#include "Move.h"

namespace Chess
{
	typedef std::function<void(const EventBase &)> GameActionListener;
	typedef std::function<void(const Move&)> BoardChangesListener;
	typedef std::vector<HistoryMove> GameHistory;
	typedef std::list<BoardChangesListener> BoardChangesListeners;

	class Game
	{
		const int AutoPlayMoveWaitSeconds = 3;
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
		const GameHistory &GetGameRecord() const;

		void Restart(bool whiteFirst = true);

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
	};
}
