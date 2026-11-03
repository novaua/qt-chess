#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

// LI-02 (docs/LI-02-ClockPlan.md): the two clocks of a game, as on lichess.org. A local game (two
// players, vs Robot) drives it with moveMade(); a Lichess game sets Lichess's times with setSnapshot()
// and never flags here (Lichess ends the game). The clock runs only after both sides have moved,
// and a move adds the increment to the mover's time if their clock was running.
// ChessConnector owns one; QML shows the texts on the player cards.
class GameClock : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
	Q_PROPERTY(QString whiteText READ whiteText NOTIFY changed)
	Q_PROPERTY(QString blackText READ blackText NOTIFY changed)
	Q_PROPERTY(bool whiteLow READ whiteLow NOTIFY changed)
	Q_PROPERTY(bool blackLow READ blackLow NOTIFY changed)
	Q_PROPERTY(bool whiteRunning READ whiteRunning NOTIFY changed)
	Q_PROPERTY(bool blackRunning READ blackRunning NOTIFY changed)

public:
	enum class Side { None, White, Black };

	explicit GameClock(QObject* parent = nullptr);

	// A new game with a clock: both sides get limitMs, nothing runs yet. localFlag: flagged() is
	// emitted when a side's time is up (a local game); a Lichess game waits for Lichess instead.
	void start(qint64 limitMs, qint64 incrementMs, bool localFlag = true);
	void stop();                                 // no clock (a game without one, or leaving the game)
	void moveMade(bool whiteMoved, int ply);     // ply: the moves played, this one included
	void setTurn(bool whiteToMove, int ply);     // e.g. after a takeback: the side to move runs from ply 2
	void setSnapshot(qint64 whiteMs, qint64 blackMs, bool whiteToMove, bool running); // Lichess's times, or a saved game's
	void setLowTimeSides(bool white, bool black); // whose low time is announced (lowTime()): the player's own
	void pause();                                // e.g. the game is over, or Continue before the page settles
	void resume();

	bool   enabled() const { return _enabled; }
	bool   paused() const { return _paused; }
	Side   runningSide() const { return _running; }
	qint64 limitMs() const { return _limitMs; }
	qint64 incrementMs() const { return _incrementMs; }
	qint64 remainingMs(bool white) const;
	qint64 lowTimeMs() const; // Lichess's: an eighth of the start time, 10 s to 60 s
	bool   isLow(bool white) const { return _enabled && remainingMs(white) < lowTimeMs(); }

	QString whiteText() const { return _enabled ? format(remainingMs(true)) : QString(); }
	QString blackText() const { return _enabled ? format(remainingMs(false)) : QString(); }
	bool whiteLow() const { return isLow(true); }
	bool blackLow() const { return isLow(false); }
	bool whiteRunning() const { return counting() && _running == Side::White; }
	bool blackRunning() const { return counting() && _running == Side::Black; }

	// As lichess.org: 03:00, 00:45, 1:30:00, tenths under 10 s (00:09.4), N days for correspondence.
	static QString format(qint64 ms);

	// Tests: the time now comes from this clock (ms); call tick() after moving it forward.
	void setClock(std::function<qint64()> clock);
	void tick();

signals:
	void changed();
	void flagged(bool white); // a local game: that side's time is up (once)
	void lowTime(bool white); // a watched side's clock went low

private:
	struct LowTimeAlarm {
		bool   watched = false;
		bool   armed = true;
		qint64 quietUntil = 0; // not again within 20 s (Lichess)
	};

	bool counting() const { return _enabled && !_paused && !_flagged && _running != Side::None; }
	qint64& stored(bool white) { return white ? _whiteMs : _blackMs; }
	void commit(qint64 now); // the running side's time so far into its stored time
	void checkLowTime(bool white, qint64 now);
	void scheduleTick();

	bool   _enabled = false;
	bool   _localFlag = true;
	bool   _paused = false;
	bool   _flagged = false;
	qint64 _limitMs = 0;
	qint64 _incrementMs = 0;
	qint64 _whiteMs = 0;     // as of _since
	qint64 _blackMs = 0;
	qint64 _since = 0;       // when the running side's time was last stored
	Side   _running = Side::None;
	LowTimeAlarm _whiteAlarm;
	LowTimeAlarm _blackAlarm;
	QString _shownWhite, _shownBlack; // what changed() last told
	bool    _shownWhiteLow = false, _shownBlackLow = false;
	Side    _shownRunning = Side::None;
	QTimer _timer;
	QElapsedTimer _elapsed;
	std::function<qint64()> _clock;
};
