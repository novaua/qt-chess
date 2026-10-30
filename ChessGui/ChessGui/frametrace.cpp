#include "stdafx.h"
#include "frametrace.h"
#include <QQuickWindow>

Q_LOGGING_CATEGORY(chessFrames, "chess.frames", QtInfoMsg)

namespace {
	// A gap this long is a visible stop (60 Hz: 16.7 ms per frame); a longer one is the window
	// resting between animations, not a stop.
	constexpr qint64 SlowFrameMs = 25;
	constexpr qint64 RestingMs = 500;
}

void FrameTrace::watch(QQuickWindow* window)
{
	if (!window || !chessFrames().isDebugEnabled())
		return;
	auto last = std::make_shared<QElapsedTimer>();
	QObject::connect(window, &QQuickWindow::afterAnimating, window, [last]() {
		if (last->isValid()) {
			const qint64 gap = last->elapsed();
			if (gap > SlowFrameMs && gap < RestingMs)
				qCDebug(chessFrames).noquote() << "[Frame trace] slow frame" << gap << "ms";
		}
		last->start();
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
