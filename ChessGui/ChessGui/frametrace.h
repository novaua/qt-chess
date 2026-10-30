#pragma once
#include <QElapsedTimer>
#include <QLoggingCategory>

class QQuickWindow;

// Frame timing of the board (BL-14), off by default. Turn it on with the environment variable
// QT_LOGGING_RULES="chess.frames.debug=true" (Release too): the app log then gets every GUI-thread
// frame gap over 25 ms while something animates (a gap over 500 ms is the window resting between two
// animations, not a stop), and how long the board's own work took (the move dots, the pieces' sync,
// the signals after a move), with the log's timestamps to match them up.
Q_DECLARE_LOGGING_CATEGORY(chessFrames)

class FrameTrace
{
public:
	// Watch the window's frames (GUI thread, between two animation steps).
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
