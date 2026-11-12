#pragma once

#include "../ChessQtTests/FakeLichess.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>

class QTcpSocket;

// BL-05 step 4c: lichess.org for the UI tests, on 127.0.0.1 (ChessQtTests' FakeLichess underneath),
// QML singleton `FakeLichess` (`import ChessGuiTests`). The player is TesterChess. It keeps what a
// test needs of Lichess: the challenges to the player, the games (their moves, draw offers, how they
// ended), the account's event stream and each game's stream, and answers the app's requests as Lichess
// does (accept, decline, a move, draw yes/no, resign, abort, claim victory, the ongoing games). A
// test plays the other side: challenge(), play(), offerDraw(), endGame(), dropGameStreams(). Open
// streams get a keep-alive line every second, as Lichess sends one every 7 s.
class FakeLichessServer : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString url READ url CONSTANT)
	Q_PROPERTY(QString username READ username CONSTANT)
	// Every request so far, "GET /api/stream/event", "POST /api/board/game/g1/move/e7e5", …
	Q_PROPERTY(QStringList requests READ requests NOTIFY requestsChanged)
	Q_PROPERTY(bool eventStreamOpen READ eventStreamOpen NOTIFY streamsChanged)

public:
	explicit FakeLichessServer(QObject* parent = nullptr);

	QString url() const { return _server.url(); }
	QString username() const { return QStringLiteral("TesterChess"); }
	QStringList requests() const { return _server.requests; }
	bool eventStreamOpen() const;

	// A challenge to the player (10+5, casual, rapid): on the event stream now, and again each time it
	// opens, until it is accepted or declined. challengerColor: the challenger's side, "white",
	// "black" or "random" (then the player gets White).
	Q_INVOKABLE void challenge(const QString& id, const QString& challenger, int rating, const QString& challengerColor);
	// An unfinished game of the player's, already under way: in /api/account/playing, and its stream
	// starts with these moves (UCI, "e2e4 e7e5"). fen: its position, for the Continue card's picture.
	Q_INVOKABLE void addOngoingGame(const QString& id, const QString& opponent, const QString& playerColor,
	                                const QString& moves, const QString& fen);
	// The opponent: a move, a draw offer, or the game's end (status "resign", "mate", "draw", …;
	// winner "white", "black" or "").
	Q_INVOKABLE void play(const QString& gameId, const QString& uciMove);
	Q_INVOKABLE void offerDraw(const QString& gameId);
	Q_INVOKABLE void endGame(const QString& gameId, const QString& status, const QString& winner);
	// The game's stream is cut, and for downMs a new one is refused (503, Lichess restarting).
	Q_INVOKABLE void dropGameStreams(const QString& gameId, int downMs);

	// What the fake has of a game: its moves (UCI, space separated), its status ("" for none).
	Q_INVOKABLE QString moves(const QString& gameId) const;
	Q_INVOKABLE QString status(const QString& gameId) const;
	Q_INVOKABLE bool received(const QString& request) const { return _server.requests.contains(request); }
	// The body of the last request made as `request` ("reason=generic"), "" if none.
	Q_INVOKABLE QString bodyOf(const QString& request) const;

signals:
	void requestsChanged();
	void streamsChanged();

private:
	struct Game
	{
		QString id;
		QString whiteName, blackName;
		QStringList moves;
		QString status = QStringLiteral("started");
		QString winner;
		bool wdraw = false, bdraw = false;
		QString fen;
		QList<QPointer<QTcpSocket>> streams;
	};

	void onRequest(QTcpSocket* socket, const QString& method, const QString& path);
	void openGameStream(QTcpSocket* socket, const QString& gameId);
	void accept(QTcpSocket* socket, const QString& challengeId);
	void boardAction(QTcpSocket* socket, const QString& gameId, const QStringList& action);
	QByteArray account() const;
	QByteArray playing() const;
	QJsonObject player(const QString& name) const;
	QByteArray gameFull(const Game& game) const;
	QJsonObject gameState(const Game& game) const;
	void sendState(Game& game);
	void sendEvent(const QJsonObject& event);
	QString playerColor(const Game& game) const; // "white" / "black"
	void keepAlive();

	LichessTests::FakeLichess _server;
	QHash<QString, QJsonObject> _challenges; // pending, by id: the event stream's "challenge" lines
	QStringList _challengeOrder;
	QHash<QString, Game> _games;
	QList<QPointer<QTcpSocket>> _eventStreams;
	qint64 _streamsDownUntil = 0; // ms since epoch: game streams answer 503 until then
	QTimer _keepAlive;
};
