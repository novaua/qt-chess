#include "stdafx.h"
#include "HistoryPlayer.h"
#include "Game.h"

using namespace Chess;

HistoryPlayer::HistoryPlayer(const GameAptr &game)
	:_game(game)
{
}

void HistoryPlayer::Play(const GameHistory &gameHistory)
{
	_history = gameHistory;
	_pointer = 0;
}

bool HistoryPlayer::CanMove(bool forward)
{
	return _history.empty()
		? false
		: forward
		? _pointer < (int)_history.size()
		: _pointer > 0;
}

void HistoryPlayer::MoveNext()
{
	if (!CanMove())
		return;

	auto move = _history[_pointer++].ToMove();
	_game->DoMove(move.From, move.To);

}

void HistoryPlayer::MoveBack()
{
	if (!CanMove(false))
		return;
	--_pointer;
	_game->UndoMove();
}

HistoryPlayer::~HistoryPlayer()
{
}
