#include "stdafx.h"
#include "gameclock.h"
#include <algorithm>

namespace {
	constexpr qint64 SecondMs = 1000;
	constexpr qint64 MinuteMs = 60 * SecondMs;
	constexpr qint64 HourMs = 60 * MinuteMs;
	constexpr qint64 DayMs = 24 * HourMs;
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
	_correspondence = false;
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

void GameClock::startCorrespondence(qint64 perMoveMs)
{
	start(perMoveMs, 0, false);
	_correspondence = true;
	tick();
}

void GameClock::stop()
{
	_enabled = false;
	_flagged = false; // the next game may have no clock: its Undo must work
	_running = Side::None;
	_timer.stop();
	tick();
}

void GameClock::moveMade(bool whiteMoved, int ply)
{
	if (!_enabled || timeIsUp())
		return;
	commit(_clock());
	const Side mover = whiteMoved ? Side::White : Side::Black;
	if (_running == mover) // the increment only once the mover's clock has run
		stored(whiteMoved) += _incrementMs;
	_running = runsAfter(ply) ? (whiteMoved ? Side::Black : Side::White) : Side::None;
	tick();
}

void GameClock::setTurn(bool whiteToMove, int ply)
{
	if (!_enabled || timeIsUp())
		return;
	commit(_clock());
	_running = runsAfter(ply) ? (whiteToMove ? Side::White : Side::Black) : Side::None;
	tick();
}

// Undo vs the robot needs nobody's consent, so the time spent isn't given back as on Lichess: that
// would be free thinking time. The increment goes, or a move and Undo over and over would add time
// for ever. Not below the last second: an Undo shouldn't lose the game by itself.
void GameClock::moveTakenBack(bool whiteMoved, int ply)
{
	if (!_enabled || ply < 3) // the first two moves got no increment
		return;
	commit(_clock());
	qint64& ms = stored(whiteMoved);
	ms = std::max(ms - _incrementMs, std::min(ms, SecondMs));
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

qint64 GameClock::remainingMs(bool white, qint64 now) const
{
	qint64 ms = white ? _whiteMs : _blackMs;
	if (runs(white))
		ms -= now - _since;
	return std::max<qint64>(0, ms);
}

qint64 GameClock::lowTimeMs() const
{
	return std::min(60 * SecondMs, _limitMs < 60 * SecondMs ? std::max(2 * SecondMs, _limitMs / 5)
	                                                        : std::max(10 * SecondMs, _limitMs / 8));
}

qint64 GameClock::moveBudgetMs(bool white) const
{
	if (!_enabled)
		return 0;
	const qint64 left = remainingMs(white);
	return std::max<qint64>(1, std::min(left / 2, std::max<qint64>(100, left / 30 + _incrementMs / 2)));
}

void GameClock::commit(qint64 now)
{
	if (counting()) {
		qint64& ms = stored(_running == Side::White);
		ms = std::max<qint64>(0, ms - (now - _since));
	}
	_since = now;
}

// The timer may be late (a busy GUI thread): a move made after the time ran out doesn't count.
bool GameClock::timeIsUp()
{
	tick();
	return _flagged;
}

QString GameClock::format(qint64 ms)
{
	ms = std::max<qint64>(0, ms);
	const qint64 hours   = ms / HourMs;
	const qint64 minutes = ms / MinuteMs % 60;
	const qint64 seconds = ms / SecondMs % 60;
	const QChar zero(u'0');
	if (hours > 0)
		return QStringLiteral("%1:%2:%3").arg(hours, 2, 10, zero).arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero);
	if (ms < TenthsBelowMs)
		return QStringLiteral("%1:%2.%3").arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero).arg(ms / 100 % 10);
	return QStringLiteral("%1:%2").arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero);
}

QString GameClock::formatCorrespondence(qint64 ms)
{
	ms = std::max<qint64>(0, ms);
	const qint64 days    = ms / DayMs;
	const qint64 hours   = ms / HourMs % 24;
	const qint64 minutes = ms / MinuteMs % 60;
	const qint64 seconds = ms / SecondMs % 60;
	const QChar zero(u'0');
	if (days > 0) {
		const QString text = days == 1 ? QStringLiteral("One day") : QStringLiteral("%1 days").arg(days);
		return hours == 0 ? text
		     : hours == 1 ? text + QStringLiteral(" 1 hour")
		                  : text + QStringLiteral(" %1 hours").arg(hours);
	}
	if (hours > 0)
		return QStringLiteral("%1:%2").arg(hours, 2, 10, zero).arg(minutes, 2, 10, zero);
	return QStringLiteral("%1:%2").arg(minutes, 2, 10, zero).arg(seconds, 2, 10, zero);
}

// Lichess's low-time sound: once below the line, again only after the time went back above 1.5
// times the line (an increment) and 20 s have passed.
void GameClock::checkLowTime(bool white, qint64 now)
{
	LowTimeAlarm& alarm = white ? _whiteAlarm : _blackAlarm;
	if (!alarm.watched || !runs(white)) // a paused clock (Continue) says nothing yet
		return;
	const qint64 ms = remainingMs(white, now);
	if (alarm.armed && low(ms) && now >= alarm.quietUntil) {
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
	const bool whiteRan = _running == Side::White;
	const bool flags = _localFlag && counting() && remainingMs(whiteRan, now) == 0;
	if (flags) {
		commit(now);
		_flagged = true;
	}
	checkLowTime(true, now);
	checkLowTime(false, now);

	const qint64 whiteMs = remainingMs(true, now);
	const qint64 blackMs = remainingMs(false, now);
	const QString white = text(whiteMs);
	const QString black = text(blackMs);
	const Side running = counting() ? _running : Side::None;
	if (white != _shownWhite || black != _shownBlack || low(whiteMs) != _shownWhiteLow
			|| low(blackMs) != _shownBlackLow || running != _shownRunning) {
		_shownWhite = white;
		_shownBlack = black;
		_shownWhiteLow = low(whiteMs);
		_shownBlackLow = low(blackMs);
		_shownRunning = running;
		emit changed();
	}
	const qint64 wait = nextTickMs(now);
	if (wait > 0)
		_timer.start(int(wait));
	else
		_timer.stop();
	if (flags)
		emit flagged(whiteRan); // last: the handler may end the game
}

// The next moment the shown time changes (a tenth, a second; correspondence: an hour, a minute), goes
// low or runs out. 0: never
// by itself, e.g. a Lichess clock at zero stays there until Lichess ends the game.
qint64 GameClock::nextTickMs(qint64 now) const
{
	const qint64 ms = counting() ? remainingMs(_running == Side::White, now) : 0;
	if (ms == 0)
		return 0;
	if (_correspondence)
		return ms % (ms >= DayMs ? HourMs : ms >= HourMs ? MinuteMs : SecondMs) + 1;
	const qint64 wait = ms % (ms < TenthsBelowMs ? 100 : SecondMs) + 1;
	return ms >= lowTimeMs() ? std::min(wait, ms - lowTimeMs() + 1) : wait;
}
