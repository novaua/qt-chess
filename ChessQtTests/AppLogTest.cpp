#include <CppUnitTest.h>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <thread>
#include <vector>
#include "applog.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// AppLog: the app logs from the GUI thread and from the engine thread, and Qt does not serialize
// message handlers. Two QTextStreams writing one QFile at once corrupted the heap (the 2026-10-31
// crash), so every line must come out whole, once, in the documented format.
namespace AppLogTests
{
	TEST_CLASS(AppLogTest)
	{
		QTemporaryDir _dir;
		QString _path = _dir.filePath("app.log");

		TEST_METHOD_INITIALIZE(Init) { AppLog::close(); }
		TEST_METHOD_CLEANUP(Cleanup)
		{
			AppLog::close();
			AppLog::setLimits(AppLog::DefaultMaxBytes, AppLog::DefaultFiles);
		}

		static QStringList linesOf(const QString& path)
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				return {};
			return QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
		}
		static void writeFile(const QString& path, const QByteArray& content)
		{
			QFile file(path);
			Assert::IsTrue(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(content);
		}
		// N of a line "… message N", -1 for another line (e.g. "Log continued from …")
		static int messageNumber(const QString& line)
		{
			static const QRegularExpression number(QStringLiteral("\\] message (\\d+)$"));
			const auto match = number.match(line);
			return match.hasMatch() ? match.captured(1).toInt() : -1;
		}
		// Every "message N" of the files, oldest first: N in order from 0, none missing.
		static int linesInOrder(const QStringList& paths)
		{
			int next = 0;
			for (const QString& path : paths)
				for (const QString& line : linesOf(path)) {
					const int n = messageNumber(line);
					if (n < 0)
						continue;
					Assert::AreEqual(next, n, line.toStdWString().c_str());
					++next;
				}
			return next;
		}

		TEST_METHOD(ConcurrentWritesKeepEveryLineWhole)
		{
			constexpr int Threads = 2;
			constexpr int PerThread = 2000;
			Assert::IsTrue(AppLog::open(_path, true));

			std::vector<std::thread> threads;
			for (int t = 0; t < Threads; ++t)
				threads.emplace_back([t] {
					for (int i = 0; i < PerThread; ++i)
						AppLog::write("DEBUG", QStringLiteral("thread %1 message %2").arg(t).arg(i));
				});
			for (auto& thread : threads)
				thread.join();
			AppLog::close();

			QFile file(_path);
			Assert::IsTrue(file.open(QIODevice::ReadOnly | QIODevice::Text));
			const QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
			Assert::AreEqual(Threads * PerThread, (int)lines.size());

			const QRegularExpression format(QStringLiteral(
				"^\\[\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}\\] \\[DEBUG\\] thread [01] message \\d+$"));
			QSet<QString> seen;
			for (const QString& line : lines) {
				Assert::IsTrue(format.match(line).hasMatch(), line.toStdWString().c_str());
				seen.insert(line);
			}
			Assert::AreEqual(Threads * PerThread, (int)seen.size());
		}

		// BL-31: a full log rolls over while the app runs: app.log, app.1.log, app.2.log (3 files in all),
		// the oldest dropped; no line cut or out of order; the new file says where the log goes on.
		TEST_METHOD(RotatesWhenFull)
		{
			AppLog::setLimits(1000, 3);
			Assert::IsTrue(AppLog::open(_path, false));
			for (int i = 0; i < 200; ++i)
				AppLog::write("DEBUG", QStringLiteral("message %1").arg(i));
			AppLog::close();

			Assert::AreEqual(_dir.filePath("app.1.log"), AppLog::rotatedPath(_path, 1));
			Assert::IsTrue(QFile::exists(AppLog::rotatedPath(_path, 1)));
			Assert::IsTrue(QFile::exists(AppLog::rotatedPath(_path, 2)));
			Assert::IsFalse(QFile::exists(AppLog::rotatedPath(_path, 3)), L"3 files in all");
			for (const QString& path : { AppLog::rotatedPath(_path, 2), AppLog::rotatedPath(_path, 1), _path }) {
				Assert::IsTrue(QFileInfo(path).size() < 1000 + 100, path.toStdWString().c_str());
				Assert::IsTrue(linesOf(path).first().endsWith(QStringLiteral("[INFO] Log continued from app.1.log")),
					path.toStdWString().c_str());
			}
			// The oldest files are gone: the kept ones go on in order up to the last line.
			const QStringList kept = linesOf(AppLog::rotatedPath(_path, 2));
			const int first = messageNumber(kept[1]);
			Assert::IsTrue(first > 0, L"the first lines were dropped with the oldest file");
			int next = first;
			for (const QString& path : { AppLog::rotatedPath(_path, 2), AppLog::rotatedPath(_path, 1), _path })
				for (const QString& line : linesOf(path))
					if (const int n = messageNumber(line); n >= 0)
						Assert::AreEqual(next++, n, line.toStdWString().c_str());
			Assert::AreEqual(200, next, L"the last line in app.log");
		}

		// A log left full by an earlier run: the next start moves it to app.1.log and begins anew.
		TEST_METHOD(OpenRotatesAnOldBigLog)
		{
			AppLog::setLimits(1000, 10);
			writeFile(_path, QByteArray(2000, 'x') + "\n");
			Assert::IsTrue(AppLog::open(_path, false));
			AppLog::write("DEBUG", QStringLiteral("message 0"));
			AppLog::close();

			Assert::AreEqual(qint64(2001), QFileInfo(AppLog::rotatedPath(_path, 1)).size(), L"the old log, as it was");
			const QStringList lines = linesOf(_path);
			Assert::AreEqual(2, (int)lines.size());
			Assert::IsTrue(lines[0].endsWith(QStringLiteral("Log continued from app.1.log")));
			Assert::AreEqual(0, messageNumber(lines[1]));
		}

		// A smaller log goes on where it was.
		TEST_METHOD(OpenAppendsBelowTheLimit)
		{
			AppLog::setLimits(1000, 10);
			writeFile(_path, "old line\n");
			Assert::IsTrue(AppLog::open(_path, false));
			AppLog::write("DEBUG", QStringLiteral("message 0"));
			AppLog::close();

			Assert::IsFalse(QFile::exists(AppLog::rotatedPath(_path, 1)));
			const QStringList lines = linesOf(_path);
			Assert::AreEqual(2, (int)lines.size());
			Assert::AreEqual(QString("old line"), lines[0]);
		}

		// A screenshot scene's log (truncate) never rolls over.
		TEST_METHOD(TruncateNeverRotates)
		{
			AppLog::setLimits(1000, 10);
			writeFile(_path, QByteArray(2000, 'x'));
			Assert::IsTrue(AppLog::open(_path, true));
			for (int i = 0; i < 50; ++i)
				AppLog::write("DEBUG", QStringLiteral("message %1").arg(i));
			AppLog::close();

			Assert::IsFalse(QFile::exists(AppLog::rotatedPath(_path, 1)));
			Assert::AreEqual(50, linesInOrder({ _path }));
		}

		// The log can't be moved (another Chess++ has it open): it is appended to, no line lost, and
		// rolls over once it can.
		TEST_METHOD(RotationBlockedKeepsLogging)
		{
			AppLog::setLimits(1000, 10);
			Assert::IsTrue(AppLog::open(_path, false));
			{
				QFile other(_path); // Qt opens it without FILE_SHARE_DELETE: it can't be renamed meanwhile
				Assert::IsTrue(other.open(QIODevice::ReadOnly));
				for (int i = 0; i < 50; ++i)
					AppLog::write("DEBUG", QStringLiteral("message %1").arg(i));
				Assert::IsFalse(QFile::exists(AppLog::rotatedPath(_path, 1)), L"blocked: not moved");
				Assert::IsTrue(QFileInfo(_path).size() > 1000, L"appended past the limit");
			}
			for (int i = 50; i < 60; ++i)
				AppLog::write("DEBUG", QStringLiteral("message %1").arg(i));
			AppLog::close();

			Assert::IsTrue(QFile::exists(AppLog::rotatedPath(_path, 1)), L"moved once it could be");
			Assert::AreEqual(60, linesInOrder({ AppLog::rotatedPath(_path, 1), _path }));
		}

		// Two threads while the log rolls over: every line whole, once, in one of the files.
		TEST_METHOD(ConcurrentWritesWhileRotating)
		{
			constexpr int Threads = 2;
			constexpr int PerThread = 2000;
			AppLog::setLimits(20000, 20);
			Assert::IsTrue(AppLog::open(_path, false));
			std::vector<std::thread> threads;
			for (int t = 0; t < Threads; ++t)
				threads.emplace_back([t] {
					for (int i = 0; i < PerThread; ++i)
						AppLog::write("DEBUG", QStringLiteral("thread %1 message %2").arg(t).arg(i));
				});
			for (auto& thread : threads)
				thread.join();
			AppLog::close();

			const QRegularExpression format(QStringLiteral(
				"^\\[\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}\\] \\[DEBUG\\] thread [01] message \\d+$"));
			QSet<QString> seen;
			int files = 0;
			for (int n = 0; n < 20; ++n) {
				const QString path = n == 0 ? _path : AppLog::rotatedPath(_path, n);
				if (!QFile::exists(path))
					continue;
				++files;
				for (const QString& line : linesOf(path)) {
					if (line.contains(QStringLiteral("[INFO] Log continued from")))
						continue;
					Assert::IsTrue(format.match(line).hasMatch(), line.toStdWString().c_str());
					seen.insert(line);
				}
			}
			Assert::IsTrue(files > 2, L"it rolled over");
			Assert::AreEqual(Threads * PerThread, (int)seen.size());
		}
	};
}
