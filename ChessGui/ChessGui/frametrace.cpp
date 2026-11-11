#include "stdafx.h"
#include "frametrace.h"
#include <QQuickWindow>
#include <QTimer>

Q_LOGGING_CATEGORY(chessFrames, "chess.frames", QtInfoMsg)

namespace {
	// A frame is due every 16 ms (60 Hz); a longer gap is one missed frame.
	constexpr int HeartbeatMs = 16;
	constexpr qint64 SlowGapMs = 25;
	// Between two animations the window doesn't render, so the next frame comes much later. Such a
	// gap is logged apart (it may also be a long stall; the GUI thread line tells which).
	constexpr qint64 RestingMs = 500;
}

void FrameTrace::watch(QQuickWindow* window)
{
	if (!window || !chessFrames().isDebugEnabled())
		return;

	// The GUI thread answers a 16 ms tick; a late tick means it was busy or blocked, however long.
	auto heartbeatStart = std::make_shared<QElapsedTimer>();
	auto* heartbeat = new QTimer(window);
	heartbeat->setTimerType(Qt::PreciseTimer);
	heartbeat->setInterval(HeartbeatMs);
	QObject::connect(heartbeat, &QTimer::timeout, window, [heartbeatStart]() {
		if (heartbeatStart->isValid()) {
			const qint64 gap = heartbeatStart->elapsed();
			if (gap > SlowGapMs)
				qCDebug(chessFrames).noquote() << "[Frame trace] GUI thread busy" << gap << "ms";
		}
		heartbeatStart->start();
	});
	heartbeat->start();

	// The frames the board drew: a gap between two of them is a stop in whatever animation ran.
	auto lastFrame = std::make_shared<QElapsedTimer>();
	QObject::connect(window, &QQuickWindow::afterAnimating, window, [lastFrame]() {
		if (lastFrame->isValid()) {
			const qint64 gap = lastFrame->elapsed();
			if (gap > RestingMs)
				qCDebug(chessFrames).noquote() << "[Frame trace] no frame for" << gap << "ms";
			else if (gap > SlowGapMs)
				qCDebug(chessFrames).noquote() << "[Frame trace] slow frame" << gap << "ms";
		}
		lastFrame->start();
	}, Qt::DirectConnection);
}

FrameTraceScope::FrameTraceScope(const char* what)
	: _what(what)
{
	if (chessFrames().isDebugEnabled())
		_timer.start();
}

FrameTraceScope::~FrameTraceScope()
{
	if (_timer.isValid())
		qCDebug(chessFrames).noquote() << "[Frame trace]" << _what
			<< QString::number(_timer.nsecsElapsed() / 1e6, 'f', 1) << "ms";
}

Q_LOGGING_CATEGORY(chessTiming, "chess.timing", QtInfoMsg)

void GameStartTiming::begin(const QString& action)
{
	disconnectWindow();
	_action = action;
	_steps.clear();
	_lastMs = 0;
	++_generation;
	_total.start();
}

void GameStartTiming::step(const char* what)
{
	if (!running())
		return;
	const qint64 now = _total.elapsed();
	_steps << QStringLiteral("%1 %2").arg(QLatin1String(what)).arg(now - _lastMs);
	_lastMs = now;
}

void GameStartTiming::shown(QQuickWindow* window, bool pageCreated)
{
	if (!running() || !window)
		return; // no window yet: the page reports again (or the timeout of a later start ends it)
	if (pageCreated)
		step("page");
	disconnectWindow();
	// Both come from the render thread and are handled on the GUI thread (the window's), queued, in
	// the order they were sent. A frame the render thread was drawing before the page existed had its
	// synchronization before shown(): its swap is skipped, and the next frame is the page's.
	_synced = false;
	const int generation = _generation;
	_syncConnection = QObject::connect(window, &QQuickWindow::afterSynchronizing, window, [this, generation]() {
		if (generation == _generation)
			_synced = true;
	}, Qt::QueuedConnection);
	_swapConnection = QObject::connect(window, &QQuickWindow::frameSwapped, window, [this, generation]() {
		if (generation == _generation && _synced && running()) {
			step("first frame");
			finish();
		}
	}, Qt::QueuedConnection);
	// A window that doesn't render (minimized) never swaps: no line, and nothing left running.
	QTimer::singleShot(5000, window, [this, generation]() {
		if (generation == _generation && running())
			cancel();
	});
	window->update();
}

void GameStartTiming::cancel()
{
	disconnectWindow();
	_total.invalidate();
	++_generation;
}

void GameStartTiming::disconnectWindow()
{
	QObject::disconnect(_syncConnection);
	QObject::disconnect(_swapConnection);
}

void GameStartTiming::finish()
{
	qCInfo(chessTiming).noquote() << QStringLiteral("[Timing] %1: %2 ms to the first frame (%3)")
		.arg(_action).arg(_total.elapsed()).arg(_steps.join(QStringLiteral(", ")));
	cancel();
}
