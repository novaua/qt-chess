#include "stdafx.h"
#include "gameclock.h"
#include <algorithm>

namespace {
	constexpr qint64 SecondMs = 1000;
	constexpr qint64 DayMs = 24 * 3600 * SecondMs;
	constexpr qint64 TenthsBelowMs = 10 * SecondMs; // tenths are shown below 10 s
	constexpr qint64 LowTimeQuietMs = 20 * SecondMs; // the low-time sound at most every 20 s
}

GameClock::GameClock(QObject* parent)
	: QObject(parent)
{
	_elapsed.start();
	_clock = [this] { return _elapsed.elapsed(); };
	_timer.setSingleShot(true);
	_timer.setTimerType(Qt::PreciseTimer);
	connect(&_timer, &QTimer::timeout, this, &GameClock::tick);
}

void GameClock::setClock(std::function<qint64()> clock)
{
	_clock = std::move(clock);
	_since = _clock();
}

void GameClock::start(qint64 limitMs, qint64 incrementMs, bool localFlag)
{
	_enabled = true;
	_localFlag = localFlag;
	_paused = false;
	_flagged = false;
	_limitMs = std::max<qint64>(0, limitMs);
	_incrementMs = std::max<qint64>(0, incrementMs);
	_whiteMs = _blackMs = _limitMs;
	_running = Side::None;
	_since = _clock();
	for (auto* alarm : { &_whiteAlarm, &_blackAlarm }) {
		alarm->armed = true;
		alarm->quietUntil = 0;
	}
	tick();
}

void GameClock::stop()
{
	_enabled = false;
	_running = Side::None;
	_timer.stop();
	tick();
}

void GameClock::moveMade(bool whiteMoved, int ply)
{
	if (!_enabled || _flagged)
		return;
	const qint64 now = _clock();
	commit(now);
	const Side mover = whiteMoved ? Side::White : Side::Black;
	if (_running == mover) // the increment only once the mover's clock has run
		stored(whiteMoved) += _incrementMs;
	_running = ply >= 2 ? (whiteMoved ? Side::Black : Side::White) : Side::None;
	tick();
}

void GameClock::setTurn(bool whiteToMove, int ply)
{
	if (!_enabled)
		return;
	commit(_clock());
	_running = ply >= 2 ? (whiteToMove ? Side::White : Side::Black) : Side::None;
	tick();
}

void GameClock::setSnapshot(qint64 whiteMs, qint64 blackMs, bool whiteToMove, bool running)
{
	if (!_enabled)
		return;
	_whiteMs = std::max<qint64>(0, whiteMs);
	_blackMs = std::max<qint64>(0, blackMs);
	_running = running ? (whiteToMove ? Side::White : Side::Black) : Side::None;
	_flagged = false; // e.g. Lichess gave time
	_since = _clock();
	tick();
}

void GameClock::setLowTimeSides(bool white, bool black)
{
	_whiteAlarm.watched = white;
	_blackAlarm.watched = black;
}

void GameClock::pause()
{
	if (_paused)
		return;
	commit(_clock());
	_paused = true;
	tick();
}

void GameClock::resume()
{
	if (!_paused)
		return;
	_paused = false;
	_since = _clock();
	tick();
}

qint64 GameClock::remainingMs(bool white) const
{
	qint64 ms = white ? _whiteMs : _blackMs;
	if (counting() && _running == (white ? Side::White : Side::Black))
		ms -= _clock() - _since;
	return std::max<qint64>(0, ms);
}

qint64 GameClock::lowTimeMs() const
{
	return std::clamp<qint64>(_limitMs / 8, 10 * SecondMs, 60 * SecondMs);
}

void GameClock::commit(qint64 now)
{
	if (counting()) {
		qint64& ms = stored(_running == Side::White);
		ms = std::max<qint64>(0, ms - (now - _since));
	}
	_since = now;
}

QString GameClock::format(qint64 ms)
{
	ms = std::max<qint64>(0, ms);
	if (ms >= DayMs) {
		const qint64 days = ms / DayMs;
		return days == 1 ? QStringLiteral("1 day") : QStringLiteral("%1 days").arg(days);
	}
	const qint64 hours   = ms / (3600 * SecondMs);
	const qint64 minutes = ms / (60 * SecondMs) % 60;
	const qint64 seconds = ms / SecondMs % 60;
	const QChar zero(u'0');
	if (hours > 0)
		return QStringLiteral("%1:%2:%3").arg(hours).arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero);
	if (ms < TenthsBelowMs)
		return QStringLiteral("%1:%2.%3").arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero).arg(ms / 100 % 10);
	return QStringLiteral("%1:%2").arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero);
}

// Lichess's low-time sound: once below the line, again only after the time went back above 1.5
// times the line (an increment) and 20 s have passed.
void GameClock::checkLowTime(bool white, qint64 now)
{
	LowTimeAlarm& alarm = white ? _whiteAlarm : _blackAlarm;
	if (!alarm.watched || !_enabled)
		return;
	const qint64 ms = remainingMs(white);
	if (alarm.armed && ms < lowTimeMs() && now >= alarm.quietUntil) {
		alarm.armed = false;
		alarm.quietUntil = now + LowTimeQuietMs;
		emit lowTime(white);
	} else if (!alarm.armed && ms > lowTimeMs() * 3 / 2) {
		alarm.armed = true;
	}
}

void GameClock::tick()
{
	const qint64 now = _clock();
	bool flaggedWhite = false;
	bool flags = false;
	if (counting()) {
		const bool white = _running == Side::White;
		if (_localFlag && remainingMs(white) == 0) {
			commit(now);
			_flagged = true;
			flags = true;
			flaggedWhite = white;
		}
	}
	checkLowTime(true, now);
	checkLowTime(false, now);

	const Side running = counting() ? _running : Side::None;
	if (whiteText() != _shownWhite || blackText() != _shownBlack || whiteLow() != _shownWhiteLow
			|| blackLow() != _shownBlackLow || running != _shownRunning) {
		_shownWhite = whiteText();
		_shownBlack = blackText();
		_shownWhiteLow = whiteLow();
		_shownBlackLow = blackLow();
		_shownRunning = running;
		emit changed();
	}
	if (flags) {
		_timer.stop();
		emit flagged(flaggedWhite); // last: the handler may end the game
		return;
	}
	scheduleTick();
}

// The next moment the shown time changes (a second, a tenth, a day), goes low or runs out.
void GameClock::scheduleTick()
{
	if (!counting()) {
		_timer.stop();
		return;
	}
	const qint64 ms = remainingMs(_running == Side::White);
	const qint64 unit = ms >= DayMs ? DayMs : ms < TenthsBelowMs ? 100 : SecondMs;
	qint64 wait = ms % unit + 1;
	if (ms >= lowTimeMs())
		wait = std::min(wait, ms - lowTimeMs() + 1);
	_timer.start(int(std::clamp<qint64>(wait, 1, DayMs)));
}
