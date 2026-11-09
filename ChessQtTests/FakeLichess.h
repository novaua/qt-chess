#pragma once
#include <QByteArray>
#include <QEventLoop>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <functional>
#include <memory>

// LI-06 / BL-10: a stand-in for lichess.org on 127.0.0.1, for the stream tests (real sockets, no
// Lichess). Answers each HTTP request with onRequest(socket, method, path); the socket stays open
// until the handler closes it (a stream).
namespace LichessTests
{
	class FakeLichess
	{
	public:
		std::function<void(QTcpSocket*, const QString& method, const QString& path)> onRequest;
		QStringList requests; // "GET /api/board/game/stream/g1", ...

		FakeLichess()
		{
			_server.listen(QHostAddress::LocalHost);
			QObject::connect(&_server, &QTcpServer::newConnection, [this]() {
				while (QTcpSocket* socket = _server.nextPendingConnection()) {
					auto received = std::make_shared<QByteArray>();
					QObject::connect(socket, &QTcpSocket::readyRead, [this, socket, received]() {
						*received += socket->readAll();
						if (socket->property("answered").toBool() || !received->contains("\r\n\r\n"))
							return;
						socket->setProperty("answered", true);
						const QList<QByteArray> requestLine = received->left(received->indexOf("\r\n")).split(' ');
						const QString method = QString::fromLatin1(requestLine.value(0));
						const QString path = QString::fromLatin1(requestLine.value(1));
						requests << method + " " + path;
						if (onRequest)
							onRequest(socket, method, path);
					});
				}
			});
		}

		QString url() const { return QStringLiteral("http://127.0.0.1:%1").arg(_server.serverPort()); }
		int count(const QString& request) const { return int(requests.count(request)); }

		static void startStream(QTcpSocket* socket)
		{
			socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\nConnection: close\r\n\r\n");
		}
		static void answer(QTcpSocket* socket, int status, const char* reason)
		{
			socket->write(QByteArray("HTTP/1.1 ") + QByteArray::number(status) + " " + reason
			              + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
			socket->disconnectFromHost();
		}
		static void line(QTcpSocket* socket, const QByteArray& json)
		{
			socket->write(json + "\n");
			socket->flush();
		}
		static QByteArray gameFull(const char* moves)
		{
			return QByteArray(R"({"type":"gameFull","id":"g1","white":{"id":"me","name":"Me"},"black":{"id":"friend","name":"Friend"},"state":{"moves":")")
				+ moves + R"(","status":"started"}})";
		}

	private:
		QTcpServer _server;
	};

	// Runs the event loop until done() or timeoutMs.
	inline void waitUntil(const std::function<bool()>& done, int timeoutMs = 5000)
	{
		QEventLoop loop;
		QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
		QTimer poll;
		poll.setInterval(20);
		QObject::connect(&poll, &QTimer::timeout, [&]() { if (done()) loop.quit(); });
		poll.start();
		loop.exec();
	}
}
