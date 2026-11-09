#include "stdafx.h"
#include "movegate.h"
#include <algorithm>

MoveGate::MoveGate(QObject* parent)
	: QObject(parent)
{
	_elapsed.start();
	_clock = [this] { return _elapsed.elapsed(); };
	_timer.setSingleShot(true);
	_timer.setTimerType(Qt::PreciseTimer);
	connect(&_timer, &QTimer::timeout, this, &MoveGate::tick);
}

void MoveGate::setTiming(const Timing& timing)
{
	_timing = timing;
	updateBusy(); // a shorter slide shortens the window already under way
	tick();
}

void MoveGate::setHoldOnline(bool hold)
{
	_holdOnline = hold;
	tick();
}

void MoveGate::setClock(std::function<qint64()> clock)
{
	_clock = std::move(clock);
}

void MoveGate::setStageReady(bool ready)
{
	if (_stageReady == ready)
		return;
	_stageReady = ready;
	if (!ready) {
		_timer.stop();
		_wasIdle = false;
		return;
	}
	tick();
}

void MoveGate::moveShown(bool byRobot)
{
	const qint64 now = _clock();
	_shownAt = now;
	_otherShownAt = now;
	if (!byRobot)
		_playerShownAt = now;
	updateBusy();
	_wasIdle = false;
	tick();
}

void MoveGate::playerMoveShown()
{
	const qint64 now = _clock();
	_shownAt = now;
	_playerShownAt = now;
	updateBusy();
	_wasIdle = false;
	tick();
}

void MoveGate::updateBusy()
{
	_busyUntil = _shownAt + _timing.slideMs + _timing.pauseMs;
	_otherBusyUntil = _otherShownAt + _timing.slideMs + _timing.pauseMs;
	_robotEarliest = _playerShownAt + _timing.robotMinMs;
}

void MoveGate::push(const QString& uciMove, Source source)
{
	_queue.push_back({ uciMove, source });
	tick();
}

// A Lichess resync: queued before the gate looks, so an idle board doesn't let the first one
// through alone.
void MoveGate::push(const QStringList& uciMoves, Source source)
{
	for (const auto& move : uciMoves)
		_queue.push_back({ move, source });
	tick();
}

void MoveGate::releaseAll()
{
	_timer.stop();
	while (!_queue.empty())
		releaseFront(_clock());
	tick();
}

void MoveGate::clear()
{
	_queue.clear();
	_busyUntil = 0;
	_otherBusyUntil = 0;
	_robotEarliest = 0;
	_shownAt = 0;
	_otherShownAt = 0;
	_playerShownAt = 0;
	tick();
}

bool MoveGate::isIdle() const
{
	return _stageReady && _queue.empty() && _clock() >= _busyUntil;
}

qint64 MoveGate::allowedAt(Source source) const
{
	if (source == Source::Robot)
		return std::max(_busyUntil, _robotEarliest);
	return _holdOnline ? _busyUntil : _otherBusyUntil; // without the hold: not for our own move's slide
}

void MoveGate::releaseFront(qint64 now)
{
	// The board is busy from here on, even if the receiver doesn't call moveShown().
	_busyUntil = std::max(_busyUntil, now + _timing.slideMs + _timing.pauseMs);
	_otherBusyUntil = std::max(_otherBusyUntil, _busyUntil);
	_wasIdle = false;
	// More than two at once is a Lichess resync after a reconnect: show the position at once,
	// as one batch, so the receiver applies it without a slide or a sound per move.
	if (_queue.size() > 2) {
		QStringList moves;
		for (const auto& move : _queue)
			moves.append(move.uciMove);
		_queue.clear();
		emit releaseBatch(moves);
	}
	else {
		const HeldMove move = _queue.front();
		_queue.pop_front();
		emit release(move.uciMove, move.source);
	}
}

void MoveGate::tick()
{
	_timer.stop();
	if (!_stageReady)
		return;

	const qint64 now = _clock();
	if (!_queue.empty()) {
		_wasIdle = false;
		const qint64 due = allowedAt(_queue.front().source);
		if (now < due) {
			_timer.start((int)(due - now));
			return;
		}
		releaseFront(now);
		tick(); // schedule what is left (moveShown() from the receiver has updated the times)
		return;
	}

	if (now < _busyUntil) {
		_wasIdle = false; // busy again (e.g. a longer slide after retiming): the edge must fire again
		_timer.start((int)(_busyUntil - now));
		return;
	}
	if (!_wasIdle) {
		_wasIdle = true;
		emit idle();
	}
}
