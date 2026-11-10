#pragma once
#include <QObject>
#include <QTimer>

// The Lichess opponent left the game: when the player may claim the win (BL-13 review). Lichess says
// how many seconds until it allows the claim (opponentGone's claimWinInSeconds, counted down here), and
// takes the claim only on the opponent's turn (lila's Pov.mightClaimWin; else "This is not the time to
// claim victory"), though it sends claimWinInSeconds 0 either way. ChessConnector owns one and tells it
// whose turn it is.
class ClaimWin : public QObject
{
	Q_OBJECT

public:
	explicit ClaimWin(QObject* parent = nullptr);

	// gone false: the opponent is back, or the game is over. seconds: until Lichess allows the
	// claim; -1 when it didn't say (allowed now).
	void setGone(bool gone, int seconds = -1);
	void setOpponentToMove(bool opponentToMove);
	void freeze();   // the screenshot mode: the countdown stands still (the same scene, the same picture)
	void tick();     // a second has passed (the timer; the tests)

	bool gone()        const { return _gone; }
	int  secondsLeft() const { return _seconds; } // -1: not gone, or not said
	bool canClaim()    const { return _gone && _seconds <= 0 && _opponentToMove; }

signals:
	void goneChanged();
	void secondsLeftChanged();
	void canClaimChanged();

private:
	void setSeconds(int seconds);
	void updateTimer();

	QTimer _timer;
	bool _gone = false;
	int  _seconds = -1;
	bool _opponentToMove = false;
	bool _frozen = false;
};
