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
