#pragma once
#include <QElapsedTimer>
#include <QLoggingCategory>

class QQuickWindow;

// Frame timing of the board (BL-14), off by default. Turn it on with the environment variable
// QT_LOGGING_RULES="chess.frames.debug=true" (Release too). The app log then gets, with timestamps:
//  - "slow frame N ms": the gap between two frames the board drew was over 25 ms (one missed frame
//    at 60 Hz), a visible stop in a slide. A gap over 500 ms is usually the window resting between
//    two animations, and is logged as "no frame for N ms" — a long stall shows up there too;
//  - "GUI thread busy N ms": a heartbeat timer (16 ms) was late, so the GUI thread was busy or
//    blocked, however long — the page-open stalls, for instance;
//  - how long the board's own work took (the move dots, the pieces' sync, the signals after a move).
Q_DECLARE_LOGGING_CATEGORY(chessFrames)

class FrameTrace
{
public:
	// Start watching the window's frames and GUI thread.
	static void watch(QQuickWindow* window);
};

// Times the work in its scope: "[Frame trace] <what> 3.2 ms".
class FrameTraceScope
{
public:
	explicit FrameTraceScope(const char* what);
	~FrameTraceScope();

	FrameTraceScope(const FrameTraceScope&) = delete;
	FrameTraceScope& operator=(const FrameTraceScope&) = delete;

private:
	const char* _what;
	QElapsedTimer _timer;
};
