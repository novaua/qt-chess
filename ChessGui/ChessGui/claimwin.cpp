#include "claimwin.h"

ClaimWin::ClaimWin(QObject* parent)
	: QObject(parent)
{
	_timer.setInterval(1000);
	connect(&_timer, &QTimer::timeout, this, &ClaimWin::tick);
}

void ClaimWin::setGone(bool gone, int seconds)
{
	if (!gone)
		seconds = -1;
	const bool could = canClaim();
	const bool goneChanges = _gone != gone;
	_gone = gone;
	if (goneChanges)
		emit goneChanged();
	setSeconds(seconds);
	if (canClaim() != could)
		emit canClaimChanged();
	updateTimer();
}

void ClaimWin::setOpponentToMove(bool opponentToMove)
{
	if (_opponentToMove == opponentToMove)
		return;
	const bool could = canClaim();
	_opponentToMove = opponentToMove;
	if (canClaim() != could)
		emit canClaimChanged();
}

void ClaimWin::freeze()
{
	_frozen = true;
	updateTimer();
}

void ClaimWin::tick()
{
	if (!_gone || _seconds <= 0)
		return;
	const bool could = canClaim();
	setSeconds(_seconds - 1);
	if (canClaim() != could)
		emit canClaimChanged();
	updateTimer();
}

void ClaimWin::setSeconds(int seconds)
{
	if (_seconds == seconds)
		return;
	_seconds = seconds;
	emit secondsLeftChanged();
}

void ClaimWin::updateTimer()
{
	const bool run = _gone && _seconds > 0 && !_frozen;
	if (!run)
		_timer.stop();
	else if (!_timer.isActive())
		_timer.start();
}
