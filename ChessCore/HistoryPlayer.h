#pragma once
#include "Move.h"

namespace Chess
{
	class Game;
	typedef std::shared_ptr<Game> GameAptr;

	class HistoryPlayer
	{
	public:
		HistoryPlayer(const GameAptr &game);

		void Play(const MovesHistory &gameHistory);

		bool CanMove(bool forward = true);

		void MoveNext();
		void MoveBack();

		~HistoryPlayer();

	private:
		GameAptr _game;
		MovesHistory _history;
		int _pointer;
	};

	typedef std::shared_ptr<HistoryPlayer> HistoryPlayerAptr;
}