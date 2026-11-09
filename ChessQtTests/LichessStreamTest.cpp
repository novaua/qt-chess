#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <CppUnitTest.h>
#include <vector>
#include "LichessStream.h"
#include "TestSupport.h"
#include "FakeLichess.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// LichessStream (BL-10 step 1, LI-06's reconnect as a class): one ndjson stream against FakeLichess.
// The owner's side (what a line means, when it counts as back) is LichessClient's; these tests play
// the owner: they call connected() on the first line after a reconnect.
namespace LichessTests
{
	TEST_CLASS(LichessStreamTest)
	{
		static const inline QString Events = QStringLiteral("GET /api/stream/event");

		QNetworkAccessManager _nam;

		static void areEqual(const QStringList& expected, const QStringList& actual)
		{
			Assert::AreEqual(expected.join(" | ").toStdWString(), actual.join(" | ").toStdWString());
		}

		QNetworkRequest request(const FakeLichess& lichess) const
		{
			return QNetworkRequest(QUrl(lichess.url() + "/api/stream/event"));
		}

		// Lines as "line <text>" ("line " for a keep-alive), and the stream's other signals.
		static void record(LichessStream& stream, QStringList& events, bool markConnected = true)
		{
			QObject::connect(&stream, &LichessStream::line, [&stream, &events, markConnected](const QByteArray& text) {
				events << "line " + QString::fromUtf8(text);
				if (markConnected)
					stream.connected();
			});
			QObject::connect(&stream, &LichessStream::connectionChanged, [&](bool connected) { events << (connected ? "back" : "lost"); });
			QObject::connect(&stream, &LichessStream::stopped, [&](bool authExpired, int status) {
				events << QString("stopped %1%2").arg(status).arg(authExpired ? " auth" : "");
			});
			QObject::connect(&stream, &LichessStream::ended, [&](QNetworkReply::NetworkError error, QString, bool) {
				events << (error == QNetworkReply::NoError ? "ended" : "ended with an error");
			});
		}

	public:
		// Not established: each line, keep-alive lines too; the end is reported, no reconnect.
		TEST_METHOD(NotEstablished_LinesThenEnded_NoReconnect)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, "a");
				FakeLichess::line(socket, "");
				FakeLichess::line(socket, "b");
				socket->disconnectFromHost();
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 100);
			QStringList events;
			record(stream, events);

			stream.open(request(lichess));
			waitUntil([&] { return events.contains("ended"); });
			waitUntil([] { return false; }, 300);

			areEqual({ "line a", "line ", "line b", "ended" }, events);
			Assert::AreEqual(1, lichess.count(Events));
			Assert::IsFalse(stream.isOpen());
		}

		TEST_METHOD(Established_Closed_ReconnectsAndIsBack)
		{
			FakeLichess lichess;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, lichess.count(Events) == 1 ? "first" : "second");
				if (lichess.count(Events) == 1)
					socket->disconnectFromHost();
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 100);
			QStringList events;
			record(stream, events);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return events.contains("back"); });

			areEqual({ "line first", "lost", "line second", "back" }, events);
			Assert::IsFalse(stream.lost());
		}

		TEST_METHOD(Established_Silent_Reconnects)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, "hello"); // then nothing, the socket open
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(300, 100);
			QStringList events;
			record(stream, events);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return events.contains("back"); });

			areEqual({ "line hello", "lost", "line hello", "back" }, events);
			Assert::AreEqual(2, lichess.count(Events));
		}

		TEST_METHOD(Established_KeepAliveLines_KeepIt)
		{
			FakeLichess lichess;
			std::vector<std::unique_ptr<QTimer>> keepAlive;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				auto timer = std::make_unique<QTimer>();
				QObject::connect(timer.get(), &QTimer::timeout, socket, [socket]() { FakeLichess::line(socket, ""); });
				timer->start(100);
				keepAlive.push_back(std::move(timer));
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(1000, 100); // ten keep-alive lines to each allowed silence
			QStringList events;
			record(stream, events, false);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([] { return false; }, 3000);

			Assert::IsFalse(events.contains("lost"));
			Assert::AreEqual(1, lichess.count(Events));
		}

		// Lichess's own "no": asking again won't change it.
		TEST_METHOD(Established_Refused_StopsTrying)
		{
			for (int status : { 404, 401 }) {
				FakeLichess lichess;
				lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
					if (lichess.count(Events) == 1) {
						FakeLichess::startStream(socket);
						FakeLichess::line(socket, "hello");
						socket->disconnectFromHost();
					}
					else {
						FakeLichess::answer(socket, status, status == 401 ? "Unauthorized" : "Not Found");
					}
				};
				LichessStream stream(_nam, "event stream");
				stream.setTiming(5000, 100);
				QStringList events;
				record(stream, events, false);

				stream.open(request(lichess));
				stream.setReconnect(true);
				waitUntil([&] { return events.join(" ").contains("stopped"); });
				waitUntil([] { return false; }, 500);

				areEqual({ "line hello", "lost", status == 401 ? "stopped 401 auth" : "stopped 404" }, events);
				Assert::AreEqual(2, lichess.count(Events));
				Assert::IsFalse(stream.isOpen());
				Assert::IsFalse(stream.lost());

				stream.reopen(); // a resync doesn't ask again either
				waitUntil([] { return false; }, 300);
				Assert::AreEqual(2, lichess.count(Events));
			}
		}

		// A server error (Lichess restarting) is worth another try.
		TEST_METHOD(Established_ServerError_TriesAgain)
		{
			FakeLichess lichess;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				const int n = lichess.count(Events);
				if (n == 2) {
					FakeLichess::answer(socket, 503, "Service Unavailable");
					return;
				}
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, "hello");
				if (n == 1)
					socket->disconnectFromHost();
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 100);
			QStringList events;
			record(stream, events);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return events.contains("back"); });

			areEqual({ "line hello", "lost", "line hello", "back" }, events);
			Assert::AreEqual(3, lichess.count(Events));
		}

		// Each failed attempt waits twice as long as the one before: 100, 200, 400 ms here.
		TEST_METHOD(Established_Backoff_Doubles)
		{
			FakeLichess lichess;
			QElapsedTimer clock;
			std::vector<qint64> at;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				at.push_back(clock.elapsed());
				FakeLichess::startStream(socket);
				if (at.size() == 1)
					FakeLichess::line(socket, "hello");
				socket->disconnectFromHost();
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 100);
			QStringList events;
			record(stream, events, false); // never back: each reconnect fails again

			clock.start();
			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return at.size() >= 4; }, 3000);

			Assert::IsTrue(at.size() >= 4);
			Assert::IsTrue(at[1] - at[0] >= 90, L"first retry after 100 ms");
			Assert::IsTrue(at[2] - at[1] >= 190, L"then 200 ms");
			Assert::IsTrue(at[3] - at[2] >= 390, L"then 400 ms");
			Assert::AreEqual(1, int(events.count("lost")), L"lost once, not per attempt");
		}

		// A resync opens it again now: not a lost connection.
		TEST_METHOD(Reopen_IsNotALoss)
		{
			FakeLichess lichess;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, QByteArray::number(lichess.count(Events)));
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 100);
			QStringList events;
			record(stream, events);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return events.contains("line 1"); });
			stream.reopen();
			waitUntil([&] { return events.contains("line 2"); });

			areEqual({ "line 1", "line 2" }, events);
		}

		// Closed while a reconnect waits: no more attempts, and reopen() does nothing.
		TEST_METHOD(Close_WhileReconnecting_StopsIt)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, "hello");
				socket->disconnectFromHost();
			};
			LichessStream stream(_nam, "event stream");
			stream.setTiming(5000, 200);
			QStringList events;
			record(stream, events, false);

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return events.contains("lost"); });
			stream.close();
			stream.reopen();
			waitUntil([] { return false; }, 600);

			Assert::AreEqual(1, lichess.count(Events));
			Assert::IsFalse(stream.isOpen());
			Assert::IsFalse(stream.lost());
		}

		// A receiver that closes the stream on a line (the game has ended): the rest isn't read.
		TEST_METHOD(CloseOnALine_StopsReading)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				socket->write("a\nend\nb\n");
				socket->flush();
			};
			LichessStream stream(_nam, "stream");
			QStringList lines;
			QObject::connect(&stream, &LichessStream::line, [&](const QByteArray& text) {
				lines << QString::fromUtf8(text);
				if (text == "end")
					stream.close();
			});

			stream.open(request(lichess));
			stream.setReconnect(true);
			waitUntil([&] { return lines.contains("end"); });
			waitUntil([] { return false; }, 300);

			areEqual({ "a", "end" }, lines);
			Assert::AreEqual(1, lichess.count(Events));
		}
	};
}
