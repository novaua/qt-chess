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
	if (!byRobot)
		_playerShownAt = now;
	updateBusy();
	_wasIdle = false;
	tick();
}

void MoveGate::updateBusy()
{
	_busyUntil = _shownAt + _timing.slideMs + _timing.pauseMs;
	_robotEarliest = _playerShownAt + _timing.robotMinMs;
}

void MoveGate::push(const QString& uciMove, Source source)
{
	_queue.push_back({ uciMove, source });
	tick();
}

void MoveGate::clear()
{
	_queue.clear();
	_busyUntil = 0;
	_robotEarliest = 0;
	_shownAt = 0;
	_playerShownAt = 0;
	tick();
}

bool MoveGate::isIdle() const
{
	return _stageReady && _queue.empty() && _clock() >= _busyUntil;
}

qint64 MoveGate::allowedAt(Source source) const
{
	return source == Source::Robot ? std::max(_busyUntil, _robotEarliest) : _busyUntil;
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
		// The board is busy from here on, even if the receiver doesn't call moveShown().
		_busyUntil = std::max(_busyUntil, now + _timing.slideMs + _timing.pauseMs);
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
		tick(); // schedule what is left (moveShown() from the receiver has updated the times)
		return;
	}

	if (now < _busyUntil) {
		_timer.start((int)(_busyUntil - now));
		return;
	}
	if (!_wasIdle) {
		_wasIdle = true;
		emit idle();
	}
}
