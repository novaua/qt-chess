#include "stdafx.h"
#include "applog.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace {
	QFile* _file = nullptr;
	// Qt calls a message handler from any thread and does not serialize the calls: the GUI thread
	// logs while the engine thread logs the robot's requests. Without this, two QTextStreams write
	// the same QFile at once and corrupt the heap (2026-10-31 crash, AppLogTest).
	QMutex _mutex;

	QString _path;              // the open log's path
	bool    _rotating = false;  // opened for appending: it rolls over (BL-31)
	qint64  _maxBytes = AppLog::DefaultMaxBytes;
	int     _files    = AppLog::DefaultFiles;
	qint64  _nextRotateAt = AppLog::DefaultMaxBytes; // the size at which the next roll-over is tried

	void writeLine(const char* level, const QString& message)
	{
		QTextStream(_file)
			<< '[' << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
			<< "] [" << level << "] " << message << '\n';
		_file->flush(); // a crash must not lose the last lines
	}

	bool openFile(QIODevice::OpenModeFlag how)
	{
		_file = new QFile(_path);
		if (!_file->open(QIODevice::WriteOnly | how | QIODevice::Text)) {
			delete _file;
			_file = nullptr;
		}
		return _file != nullptr;
	}

	void closeFile()
	{
		if (!_file)
			return;
		_file->flush();
		delete _file;
		_file = nullptr;
	}

	// The files move up one ("name.log" → "name.1.log" …), the oldest goes. With the mutex held and
	// the log closed. False when the current file couldn't be moved (another Chess++ has it open):
	// it is then appended to, and tried again once it has grown by a tenth of the limit.
	bool shiftFiles()
	{
		QFile::remove(AppLog::rotatedPath(_path, _files - 1));
		for (int n = _files - 2; n >= 1; --n) {
			const QString from = AppLog::rotatedPath(_path, n);
			if (QFile::exists(from))
				QFile::rename(from, AppLog::rotatedPath(_path, n + 1));
		}
		return _files <= 1 ? QFile::remove(_path) : QFile::rename(_path, AppLog::rotatedPath(_path, 1));
	}

	// The current log is full: a new one (with the mutex held).
	void rotate()
	{
		closeFile();
		if (shiftFiles() && openFile(QIODevice::Truncate)) {
			_nextRotateAt = _maxBytes;
			if (_files > 1)
				writeLine("INFO", QStringLiteral("Log continued from ")
					+ QFileInfo(AppLog::rotatedPath(_path, 1)).fileName());
			return;
		}
		if (!_file)
			openFile(QIODevice::Append); // not moved: no line is lost
		_nextRotateAt = (_file ? _file->size() : 0) + qMax<qint64>(_maxBytes / 10, 1);
	}
}

QString AppLog::rotatedPath(const QString& path, int n)
{
	const QFileInfo info(path);
	const QString suffix = info.suffix().isEmpty() ? QString() : QLatin1Char('.') + info.suffix();
	return info.path() + QLatin1Char('/') + info.completeBaseName() + QLatin1Char('.') + QString::number(n) + suffix;
}

bool AppLog::open(const QString& path, bool truncate)
{
	QMutexLocker lock(&_mutex);
	closeFile();
	_path = path;
	_rotating = !truncate;
	_nextRotateAt = _maxBytes;
	if (truncate)
		return openFile(QIODevice::Truncate);
	// A log left full by the last run starts a new one now.
	if (_maxBytes > 0 && QFileInfo(path).size() >= _maxBytes)
		rotate();
	else
		openFile(QIODevice::Append);
	return _file != nullptr;
}

void AppLog::close()
{
	QMutexLocker lock(&_mutex);
	closeFile();
}

void AppLog::write(const char* level, const QString& message)
{
	QMutexLocker lock(&_mutex);
	if (!_file)
		return;
	writeLine(level, message);
	if (_rotating && _maxBytes > 0 && _file->size() >= _nextRotateAt)
		rotate();
}

void AppLog::setLimits(qint64 maxBytes, int files)
{
	QMutexLocker lock(&_mutex);
	_maxBytes = maxBytes;
	_files = qMax(files, 1);
	_nextRotateAt = maxBytes;
}
