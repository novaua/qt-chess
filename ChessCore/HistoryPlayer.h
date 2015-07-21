#pragma once
#include "Move.h"

namespace Chess
{
	class Game;

	class HistoryPlayer
	{
	public:
		HistoryPlayer(Game *game);

		void Play(const GameHistory &gameHistory);

		bool CanMove(bool forward = true);

		void MoveNext();
		void MoveBack();

		~HistoryPlayer();

	private:
		Game *_game;
		GameHistory _history;
		int _pointer;
	};

	typedef std::shared_ptr<HistoryPlayer> HistoryPlayerAptr;
}