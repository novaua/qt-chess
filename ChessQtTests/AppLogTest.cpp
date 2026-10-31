#include <CppUnitTest.h>
#include <QFile>
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
		TEST_METHOD_CLEANUP(Cleanup) { AppLog::close(); }

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
	};
}
