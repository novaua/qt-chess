#pragma once
#include <QString>
#include <QtGlobal>

// The app's log file. Qt calls a message handler from any thread and does not serialize the calls,
// so the GUI thread and the engine thread can log at the same time: write() must be thread-safe.
// (Two QTextStreams writing one QFile at once corrupted the heap — crash of 2026-10-31.)
// BL-31: a log opened for appending rolls over once it reaches the size limit (at open, and while the
// app runs): "name.log" becomes "name.1.log", the older ones move up one, and only `files` in all are
// kept (the current one too), the oldest deleted. A log opened with truncate (a screenshot scene's)
// never rolls over.
class AppLog
{
public:
	static constexpr qint64 DefaultMaxBytes = 5 * 1024 * 1024;
	static constexpr int    DefaultFiles    = 10;

	static bool open(const QString& path, bool truncate); // false when the file can't be opened
	static void close();
	// One line: "[yyyy-MM-dd hh:mm:ss.zzz] [LEVEL] message".
	static void write(const char* level, const QString& message);
	// The roll-over limits (the tests' small ones); maxBytes <= 0: never. Takes effect at the next line.
	static void setLimits(qint64 maxBytes, int files);
	// "dir/name.n.log" of "dir/name.log": the n-th older file (1: the newest of them).
	static QString rotatedPath(const QString& path, int n);
};
