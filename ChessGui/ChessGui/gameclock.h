#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <functional>

// LI-02 (docs/LI-02-ClockPlan.md): the two clocks of a game, as on lichess.org. A local game (two
// players, vs Robot) drives it with moveMade(); a Lichess game sets Lichess's times with setSnapshot()
// and never flags here (Lichess ends the game). The clock runs only after both sides have moved,
// and a move adds the increment to the mover's time if their clock was running. A correspondence
// game's clock is the time left for the move: it runs from the start, is never low, and shows days.
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
	Q_PROPERTY(bool timeUp READ timeUp NOTIFY changed)

public:
	explicit GameClock(QObject* parent = nullptr);

	// A new game with a clock: both sides get limitMs, nothing runs yet. localFlag: flagged() is
	// emitted when a side's time is up (a local game); a Lichess game waits for Lichess instead.
	void start(qint64 limitMs, qint64 incrementMs, bool localFlag = true);
	void startCorrespondence(qint64 perMoveMs);  // a Lichess game with days per move
	void stop();                                 // no clock (a game without one, or leaving the game)
	void moveMade(bool whiteMoved, int ply);     // ply: the moves played, this one included
	void setTurn(bool whiteToMove, int ply);     // e.g. after a takeback: the side to move runs from ply 2; not after a flag
	void moveTakenBack(bool whiteMoved, int ply); // before setTurn(), per move: its increment goes, the time spent stays spent
	void setSnapshot(qint64 whiteMs, qint64 blackMs, bool whiteToMove, bool running); // Lichess's times, or a saved game's
	void setLowTimeSides(bool white, bool black); // whose low time is announced (lowTime()): the player's own
	void pause();                                // e.g. the game is over, or Continue before the page settles; local games only: Lichess's time goes on
	void resume();

	bool   enabled() const { return _enabled; }
	qint64 limitMs() const { return _limitMs; }
	qint64 incrementMs() const { return _incrementMs; }
	qint64 remainingMs(bool white) const { return remainingMs(white, _clock()); }
	qint64 lowTimeMs() const; // Lichess's: an eighth of the start time, 10 s to 60 s (a fifth, from 2 s, below a minute)
	// The robot's thinking time on this clock: a thirtieth of what's left plus half the increment, at
	// least 0.1 s, but never more than half of what's left. 0: no clock.
	qint64 moveBudgetMs(bool white) const;
	bool   timeUp() const { return _flagged; } // a local game: a side has flagged
	bool   isLow(bool white) const { return low(remainingMs(white)); }

	QString whiteText() const { return text(remainingMs(true)); }
	QString blackText() const { return text(remainingMs(false)); }
	bool whiteLow() const { return isLow(true); }
	bool blackLow() const { return isLow(false); }
	bool whiteRunning() const { return runs(true); }
	bool blackRunning() const { return runs(false); }

	// As lichess.org: 03:00, 00:45, 01:30:00, tenths under 10 s (00:09.4).
	static QString format(qint64 ms);
	// As lichess.org's correspondence clock: 2 days 4 hours, One day, 05:30 (hours and minutes), 45:10.
	static QString formatCorrespondence(qint64 ms);

	// Tests: the time now comes from this clock (ms); call tick() after moving it forward.
	void setClock(std::function<qint64()> clock);
	void tick();
	qint64 nextTickMs() const { return nextTickMs(_clock()); } // when the timer wakes; 0: it sleeps

signals:
	void changed();
	void flagged(bool white); // a local game: that side's time is up (once)
	void lowTime(bool white); // a watched side's clock went low

private:
	enum class Side { None, White, Black };

	struct LowTimeAlarm {
		bool   watched = false;
		bool   armed = true;
		qint64 quietUntil = 0; // not again within 20 s (Lichess)
	};

	bool counting() const { return _enabled && !_paused && !_flagged && _running != Side::None; }
	bool runs(bool white) const { return counting() && _running == (white ? Side::White : Side::Black); }
	qint64& stored(bool white) { return white ? _whiteMs : _blackMs; }
	qint64 remainingMs(bool white, qint64 now) const;
	qint64 nextTickMs(qint64 now) const;
	bool runsAfter(int ply) const { return ply >= 2 || _correspondence; }
	QString text(qint64 ms) const { return !_enabled ? QString() : _correspondence ? formatCorrespondence(ms) : format(ms); }
	bool low(qint64 ms) const { return _enabled && !_correspondence && ms < lowTimeMs(); }
	void commit(qint64 now); // the running side's time so far into its stored time
	bool timeIsUp();
	void checkLowTime(bool white, qint64 now);

	bool   _enabled = false;
	bool   _localFlag = true;
	bool   _correspondence = false;
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
