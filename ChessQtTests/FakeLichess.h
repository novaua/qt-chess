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
		QStringList bodies;   // their bodies, in the same order ("reason=tooFast"; "" for a GET)
		QStringList dropped;  // the requests whose connection the client closed (not answer(), json(), close())

		FakeLichess()
		{
			_server.listen(QHostAddress::LocalHost);
			QObject::connect(&_server, &QTcpServer::newConnection, [this]() {
				while (QTcpSocket* socket = _server.nextPendingConnection()) {
					auto received = std::make_shared<QByteArray>();
					QObject::connect(socket, &QTcpSocket::readyRead, [this, socket, received]() {
						*received += socket->readAll();
						const int headEnd = int(received->indexOf("\r\n\r\n"));
						if (socket->property("answered").toBool() || headEnd < 0)
							return;
						int length = 0; // the body, if any, may come in a later packet
						for (const QByteArray& header : received->left(headEnd).split('\n'))
							if (header.toLower().startsWith("content-length:"))
								length = header.mid(15).trimmed().toInt();
						if (received->size() < headEnd + 4 + length)
							return;
						socket->setProperty("answered", true);
						bodies << QString::fromUtf8(received->mid(headEnd + 4, length));
						const QList<QByteArray> requestLine = received->left(received->indexOf("\r\n")).split(' ');
						const QString method = QString::fromLatin1(requestLine.value(0));
						const QString path = QString::fromLatin1(requestLine.value(1));
						requests << method + " " + path;
						socket->setProperty("request", method + " " + path);
						QObject::connect(socket, &QTcpSocket::disconnected, socket, [this, socket]() {
							if (!socket->property("closedHere").toBool())
								dropped << socket->property("request").toString();
						});
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
			close(socket);
		}
		static void json(QTcpSocket* socket, const QByteArray& body, int status = 200)
		{
			socket->write("HTTP/1.1 " + QByteArray::number(status) + (status == 200 ? " OK" : " Refused")
			              + "\r\nContent-Type: application/json\r\nContent-Length: "
			              + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
			close(socket);
		}
		// Any other content: an image (LI-11's flair), "image/webp".
		static void bytes(QTcpSocket* socket, const QByteArray& body, const char* contentType)
		{
			socket->write(QByteArray("HTTP/1.1 200 OK\r\nContent-Type: ") + contentType + "\r\nContent-Length: "
			              + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
			close(socket);
		}
		// Lichess ends a stream (a seek taken or expired): not the client's doing.
		static void close(QTcpSocket* socket)
		{
			socket->setProperty("closedHere", true);
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

	// LI-09: Lichess's side of quick pairing. The account's event stream; POST /api/board/seek held open
	// with a keep-alive line until pair() or endSeek() (a correspondence seek, or one refused with
	// seekStatus, answered at once); Your games (`playing`); a game stream per game (a game id starting
	// "gone": 404, the game is over); an accept says ok; a challenge to a player held open (waiting).
	class FakeSeekLichess: public FakeLichess
	{
	public:
		QTcpSocket* events = nullptr;     // the event stream
		QTcpSocket* seekSocket = nullptr; // the real-time seek's request, while open
		QByteArray  playing = R"({"nowPlaying":[]})"; // GET /api/account/playing
		int         seekStatus = 200;     // else the seek is refused with seekError
		QByteArray  seekError = R"({"error":"Invalid speed"})";
		QByteArray  gameFullFields;       // more of a game's gameFull, e.g. R"("rated":true,"clock":{…},)"

		static inline const QString Seek = QStringLiteral("POST /api/board/seek");

		FakeSeekLichess()
		{
			onRequest = [this](QTcpSocket* socket, const QString& method, const QString& path) {
				if (path == "/api/stream/event") {
					startStream(socket);
					events = socket;
				} else if (method == "POST" && path == "/api/board/seek") {
					if (seekStatus != 200)
						json(socket, seekError, seekStatus);
					else if (bodies.last().contains("days="))
						json(socket, R"({"id":"corr0001"})");
					else {
						// Chunked, as Lichess's stream: a clean end is the last chunk (endSeek), and a
						// connection dropped without it (abort) is a network error to the client.
						socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/x-ndjson\r\n"
						              "Transfer-Encoding: chunked\r\n\r\n");
						socket->write("1\r\n\n\r\n"); // keep-alive: an empty line
						socket->flush();
						seekSocket = socket;
						QObject::connect(socket, &QTcpSocket::disconnected, socket, [this, socket]() {
							if (seekSocket == socket)
								seekSocket = nullptr;
						});
					}
				} else if (path.startsWith("/api/account/playing")) {
					json(socket, playing);
				} else if (path.startsWith("/api/board/game/stream/gone")) {
					answer(socket, 404, "Not Found");
				} else if (method == "POST" && path.startsWith("/api/challenge/") && !path.endsWith("/accept")) {
					startStream(socket); // keepAliveStream: the challenge waits while this stays open
				} else if (path.startsWith("/api/board/game/stream/")) {
					startStream(socket);
					line(socket, QByteArray(R"({"type":"gameFull","id":")") + path.mid(23).toLatin1()
						+ R"(","speed":"rapid","white":{"id":"me","name":"Me"},"black":{"id":"stranger","name":"Stranger"},)"
						+ gameFullFields + R"("state":{"moves":"","status":"started"}})");
				} else if (method == "POST" && path.endsWith("/accept")) {
					json(socket, R"({"ok":true})");
				} else {
					answer(socket, 404, "Not Found");
				}
			};
		}

		// A gameStart event as Lichess sends it on the event stream.
		static QByteArray gameStart(const QString& gameId, const char* source, const char* speed = "rapid",
		                            bool rated = true, const char* variant = "standard", bool hasMoved = false)
		{
			return R"({"type":"gameStart","game":{"gameId":")" + gameId.toLatin1() + R"(","fullId":")" + gameId.toLatin1()
				+ R"(abcd","color":"white","source":")" + source + R"(","speed":")" + speed
				+ R"(","rated":)" + (rated ? "true" : "false") + R"(,"hasMoved":)" + (hasMoved ? "true" : "false")
				+ R"(,"variant":{"key":")" + variant + R"("},"opponent":{"id":"stranger","username":"Stranger"}}})";
		}
		// Someone took the seek: its game starts (gameStart), and Lichess closes the seek's request.
		void pair(const QString& gameId, const char* source = "lobby")
		{
			line(events, gameStart(gameId, source));
			endSeek();
		}
		// Lichess closes the seek's request: taken (without the gameStart, as if it was missed) or expired.
		void endSeek()
		{
			if (seekSocket) {
				seekSocket->write("0\r\n\r\n"); // the last chunk: Lichess ended the request cleanly
				close(seekSocket);
			}
		}
		// A row of Your games (/api/account/playing).
		static QByteArray playingGame(const char* gameId, const char* speed = "rapid", const char* source = "lobby",
		                              bool rated = true)
		{
			return QByteArray(R"({"gameId":")") + gameId + R"(","fullId":")" + gameId + R"(abcd","color":"white","isMyTurn":true,)"
				+ R"("speed":")" + speed + R"(","source":")" + source + R"(","rated":)" + (rated ? "true" : "false")
				+ R"(,"hasMoved":false,"variant":{"key":"standard"},"opponent":{"username":"Stranger"}})";
		}
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
