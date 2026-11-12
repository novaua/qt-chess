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
		// A premove (LI-05): the squares the piece on that square might go to once the other side has
		// made one move. By the piece's shape, without checks: a line stops at our own piece (it may be
		// taken, then we take back) and passes one of theirs (it may move away). Whether the move is
		// legal is decided when it is played.
		std::vector<BoardPosition> GetPremoveTargets(int index) const;
		bool IsPromotion(int from, int to) const; // the piece there is a pawn and `to` is on the last rank

		Piece GetPieceAt(int index) const;
		const Board& GetCurrentBoard() const { return *_boardAptr; }

		void Restart(bool whiteFirst = true);
		void EndGame();

		const MovesHistory &GetGameRecord() const;
		HistoryPlayerAptr MakePlayer();

		bool Save(const std::string &path); // false: the file couldn't be written
		void Load(const std::string &path);
		void ResumeFromLoad();

		void DoMove(BoardPosition from, BoardPosition to);
		void DoMove(const Move &move);
		void UndoMove();

		bool IsWhiteMove();
		bool IsCheckMate() const { return _checkMate; } // the last move mated
		bool IsInCheck();                               // the side to move is in check
		bool IsStalemate();                             // the side to move isn't in check and has no legal move
		int GetMoveCount();
		int GetHalfMovesCount();

		BoardPosition ElPasantPosition();

		std::string MakeFen();

		// SAN of every move of a game from the start position, with '+' and '#'
		// (replays the game to find the mate).
		static std::vector<std::string> FormatHistorySan(const MovesHistory& history);

		~Game();
	private:
		bool CanCastleKingside(PieceColors c) const;
		bool CanCastleQueenside(PieceColors c) const;

		void AssureMove(BoardPosition from, BoardPosition to);
		bool CanMoveFrom(BoardPosition from);
		void NotifyFullBoardReinit();
		void NotifyCheckState(PieceColors movedSide);

		void NotifyBoardChangesListeners(std::vector<BoardPosition> indexes);
		void NotifyActionsListeners(const EventBase &event);
	};

	typedef std::shared_ptr<Game> GameAptr;
}
