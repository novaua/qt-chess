#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>
#include <deque>
#include <functional>

// BL-14 (docs/BL-14-MovePacingPlan.md): holds the moves that come from outside — the robot's
// answer, the Lichess opponent's move — and lets them through one at a time, only when the board is
// idle: the page has settled and isn't turning (setStageReady), the last slide is over plus a short
// pause (moveShown), and the robot's answer also waits a little longer so it doesn't look instant.
// ChessConnector owns one and plays each move when release() comes, then calls moveShown().
class MoveGate : public QObject
{
	Q_OBJECT

public:
	enum class Source { Robot, Online };

	struct Timing {
		int slideMs = 250;    // Theme.slow: one piece's slide
		int pauseMs = 150;    // a beat after the slide
		int robotMinMs = 400; // the robot's answer never comes right after the player's move
	};

	explicit MoveGate(QObject* parent = nullptr);

	void setTiming(const Timing& timing);
	Timing timing() const { return _timing; }
	void setStageReady(bool ready);       // the page has settled and the board isn't turning
	void moveShown(bool byRobot = false); // a move was played: the board is busy for slideMs + pauseMs
	void push(const QString& uciMove, Source source);
	void clear();                         // new game, takeback, leaving the game, review
	bool holding() const { return !_queue.empty(); }

	// Tests: the time now comes from this clock (ms); call tick() after moving it forward.
	void setClock(std::function<qint64()> clock);
	void tick();

signals:
	void release(const QString& uciMove, MoveGate::Source source);
	void idle(); // nothing waits and the board has settled

private:
	struct HeldMove { QString uciMove; Source source; };
	qint64 allowedAt(Source source) const;
	void schedule();

	Timing _timing;
	std::deque<HeldMove> _queue;
	QTimer _timer;
	QElapsedTimer _elapsed;
	std::function<qint64()> _clock;
	qint64 _busyUntil = 0;
	qint64 _robotEarliest = 0;
	bool _stageReady = false;
	bool _wasIdle = false;
};
