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
	tick(); // the slide may have become shorter
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
	_busyUntil = now + _timing.slideMs + _timing.pauseMs;
	if (!byRobot)
		_robotEarliest = now + _timing.robotMinMs;
	_wasIdle = false;
	tick();
}

void MoveGate::push(const QString& uciMove, Source source)
{
	_queue.push_back({ uciMove, source });
	tick();
}

void MoveGate::clear()
{
	_queue.clear();
	tick();
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
		// More than two at once is a Lichess resync after a reconnect: show the position at once.
		if (_queue.size() > 2) {
			const auto moves = std::move(_queue);
			_queue.clear();
			for (const auto& move : moves)
				emit release(move.uciMove, move.source);
		}
		else {
			const HeldMove move = _queue.front();
			_queue.pop_front();
			emit release(move.uciMove, move.source);
		}
		schedule(); // the receiver played it (moveShown) or not: wake up for what is left
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

void MoveGate::schedule()
{
	_timer.stop();
	if (!_stageReady)
		return;
	const qint64 now = _clock();
	if (!_queue.empty()) {
		const qint64 due = allowedAt(_queue.front().source);
		_timer.start(due <= now ? 0 : (int)(due - now));
	}
	else if (_busyUntil > now) {
		_timer.start((int)(_busyUntil - now));
	}
}
