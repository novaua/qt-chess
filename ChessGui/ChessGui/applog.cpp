#include "stdafx.h"
#include "applog.h"
#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace {
	QFile* _file = nullptr;
	// Qt calls a message handler from any thread and does not serialize the calls: the GUI thread
	// logs while the engine thread logs the robot's requests. Without this, two QTextStreams write
	// the same QFile at once and corrupt the heap (2026-10-31 crash, AppLogTest).
	QMutex _mutex;
}

bool AppLog::open(const QString& path, bool truncate)
{
	QMutexLocker lock(&_mutex);
	if (_file) {
		_file->flush();
		delete _file;
	}
	_file = new QFile(path);
	const auto mode = QIODevice::WriteOnly
		| (truncate ? QIODevice::Truncate : QIODevice::Append)
		| QIODevice::Text;
	if (!_file->open(mode)) {
		delete _file;
		_file = nullptr;
	}
	return _file != nullptr;
}

void AppLog::close()
{
	QMutexLocker lock(&_mutex);
	if (!_file)
		return;
	_file->flush();
	delete _file;
	_file = nullptr;
}

void AppLog::write(const char* level, const QString& message)
{
	QMutexLocker lock(&_mutex);
	if (!_file)
		return;
	QTextStream(_file)
		<< '[' << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
		<< "] [" << level << "] " << message << '\n';
	_file->flush(); // a crash must not lose the last lines
}
