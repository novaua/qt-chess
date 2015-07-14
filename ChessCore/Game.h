#pragma once
#include "Board.h"

namespace Chess
{
	class Game;
	typedef std::function<void()> Action;
	typedef std::shared_ptr<Game> GameAptr;

	class Game
	{
	public:
		Game();
		static GameAptr StartNewGame(bool whiteFirst);
		 
		virtual void CheckNotify(const Action &listener) = 0;
		virtual void MateNotify(const Action &listener) = 0;
		virtual void EnPassantNotify(const Action &listener) = 0;
		virtual void PawnPromotionNotify(const std::function<void(int)> &listener) = 0;
		 
		virtual void RegisterBoardChanged(const std::function<Move> &listener) = 0;
		 
		virtual std::vector<Move> GetPossibleMoves(int index) = 0;
		virtual std::vector<Move> GetGameRecord() = 0;
		 
		virtual void Save(const std::string &path) = 0;
		virtual void Load(const std::string &path) = 0;
		 
		virtual void DoMove(EMoves from, EMoves to) = 0;
		virtual void UndoMove() = 0;
		virtual ~Game();
	};
}
