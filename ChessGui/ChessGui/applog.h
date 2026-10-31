#pragma once
#include <QString>

// The app's log file. Qt calls a message handler from any thread and does not serialize the calls,
// so the GUI thread and the engine thread can log at the same time: write() must be thread-safe.
// (Two QTextStreams writing one QFile at once corrupted the heap — crash of 2026-10-31.)
class AppLog
{
public:
	static bool open(const QString& path, bool truncate); // false when the file can't be opened
	static void close();
	// One line: "[yyyy-MM-dd hh:mm:ss.zzz] [LEVEL] message".
	static void write(const char* level, const QString& message);
};
