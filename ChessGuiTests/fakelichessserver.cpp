#include "fakelichessserver.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpSocket>

using LichessTests::FakeLichess;

static QByteArray compact(const QJsonObject& object)
{
	return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

static void ok(QTcpSocket* socket)
{
	FakeLichess::json(socket, R"({"ok":true})");
}

FakeLichessServer::FakeLichessServer(QObject* parent)
	: QObject(parent)
{
	_server.onRequest = [this](QTcpSocket* socket, const QString& method, const QString& path) {
		onRequest(socket, method, path);
		emit requestsChanged();
	};
	_keepAlive.setInterval(1000);
	connect(&_keepAlive, &QTimer::timeout, this, &FakeLichessServer::keepAlive);
	_keepAlive.start();
}

bool FakeLichessServer::eventStreamOpen() const
{
	for (const auto& socket : _eventStreams)
		if (socket && socket->state() == QAbstractSocket::ConnectedState)
			return true;
	return false;
}

// ── What a test does as the other side ─────────────────────────────────

void FakeLichessServer::challenge(const QString& id, const QString& challenger, int rating, const QString& challengerColor)
{
	const QJsonObject event{
		{ "type", "challenge" },
		{ "compat", QJsonObject{ { "bot", false }, { "board", true } } },
		{ "challenge", QJsonObject{
			{ "id", id },
			{ "url", "https://lichess.org/" + id },
			{ "status", "created" },
			{ "direction", "in" },
			{ "challenger", QJsonObject{ { "id", challenger.toLower() }, { "name", challenger },
			                             { "rating", rating }, { "provisional", false } } },
			{ "destUser", player(username()) },
			{ "variant", QJsonObject{ { "key", "standard" } } },
			{ "rated", false },
			{ "speed", "rapid" },
			{ "timeControl", QJsonObject{ { "type", "clock" }, { "limit", 600 }, { "increment", 5 }, { "show", "10+5" } } },
			{ "color", challengerColor },
		} },
	};
	if (!_challenges.contains(id))
		_challengeOrder << id;
	_challenges.insert(id, event);
	sendEvent(event);
}

void FakeLichessServer::addOngoingGame(const QString& id, const QString& opponent, const QString& playerColor,
                                       const QString& moves, const QString& fen)
{
	Game game;
	game.id = id;
	const bool white = playerColor == QLatin1String("white");
	game.whiteName = white ? username() : opponent;
	game.blackName = white ? opponent : username();
	game.moves = moves.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	game.fen = fen;
	_games.insert(id, game);
}

void FakeLichessServer::play(const QString& gameId, const QString& uciMove)
{
	auto game = _games.find(gameId);
	if (game == _games.end())
		return;
	game->moves << uciMove;
	game->wdraw = game->bdraw = false; // a move declines an offer
	sendState(*game);
}

void FakeLichessServer::offerDraw(const QString& gameId)
{
	auto game = _games.find(gameId);
	if (game == _games.end())
		return;
	(playerColor(*game) == QLatin1String("white") ? game->bdraw : game->wdraw) = true;
	sendState(*game);
}

void FakeLichessServer::endGame(const QString& gameId, const QString& status, const QString& winner)
{
	auto game = _games.find(gameId);
	if (game == _games.end())
		return;
	game->status = status;
	game->winner = winner;
	sendState(*game);
	sendEvent(QJsonObject{ { "type", "gameFinish" }, { "game", QJsonObject{ { "gameId", gameId } } } });
}

void FakeLichessServer::dropGameStreams(const QString& gameId, int downMs)
{
	_streamsDownUntil = QDateTime::currentMSecsSinceEpoch() + downMs;
	auto game = _games.find(gameId);
	if (game == _games.end())
		return;
	for (const auto& socket : game->streams)
		if (socket)
			socket->abort(); // as a lost connection: no clean end of the reply
	game->streams.clear();
	emit streamsChanged();
}

QString FakeLichessServer::moves(const QString& gameId) const
{
	return _games.value(gameId).moves.join(QLatin1Char(' '));
}

QString FakeLichessServer::status(const QString& gameId) const
{
	return _games.contains(gameId) ? _games.value(gameId).status : QString();
}

QString FakeLichessServer::bodyOf(const QString& request) const
{
	const int i = int(_server.requests.lastIndexOf(request));
	return i < 0 ? QString() : _server.bodies.value(i);
}

// ── Lichess's answers ─────────────────────────────────────────────────

void FakeLichessServer::onRequest(QTcpSocket* socket, const QString& method, const QString& fullPath)
{
	const QString path = fullPath.section(QLatin1Char('?'), 0, 0);
	const QStringList parts = path.split(QLatin1Char('/'), Qt::SkipEmptyParts); // "api", …
	const bool get = method == QLatin1String("GET");

	if (get && path == QLatin1String("/api/account"))
		return FakeLichess::json(socket, account());
	if (get && path == QLatin1String("/api/account/playing"))
		return FakeLichess::json(socket, playing());
	if (get && path == QLatin1String("/api/stream/event")) {
		FakeLichess::startStream(socket);
		_eventStreams << socket;
		connect(socket, &QTcpSocket::disconnected, this, &FakeLichessServer::streamsChanged);
		for (const QString& id : _challengeOrder) // as Lichess: the challenges waiting, when it opens
			FakeLichess::line(socket, compact(_challenges.value(id)));
		emit streamsChanged();
		return;
	}
	// /api/challenge/<id>/accept, /api/challenge/<id>/decline
	if (!get && parts.size() == 4 && parts[1] == QLatin1String("challenge")) {
		if (parts[3] == QLatin1String("accept"))
			return accept(socket, parts[2]);
		if (parts[3] == QLatin1String("decline")) {
			_challenges.remove(parts[2]);
			_challengeOrder.removeAll(parts[2]);
			return ok(socket);
		}
	}
	// /api/board/game/stream/<id>
	if (get && parts.size() == 5 && parts[1] == QLatin1String("board") && parts[3] == QLatin1String("stream"))
		return openGameStream(socket, parts[4]);
	// /api/board/game/<id>/<action>[/<arg>]
	if (!get && parts.size() >= 5 && parts[1] == QLatin1String("board") && parts[2] == QLatin1String("game"))
		return boardAction(socket, parts[3], parts.mid(4));
	FakeLichess::answer(socket, 404, "Not Found");
}

void FakeLichessServer::accept(QTcpSocket* socket, const QString& challengeId)
{
	if (!_challenges.contains(challengeId))
		return FakeLichess::answer(socket, 404, "Not Found");
	const QJsonObject challenge = _challenges.take(challengeId).value("challenge").toObject();
	_challengeOrder.removeAll(challengeId);
	const QString challenger = challenge.value("challenger").toObject().value("name").toString();
	const bool challengerWhite = challenge.value("color").toString() == QLatin1String("white");
	Game game;
	game.id = challengeId; // as on Lichess: the challenge's id is its game's
	game.whiteName = challengerWhite ? challenger : username();
	game.blackName = challengerWhite ? username() : challenger;
	_games.insert(challengeId, game);
	ok(socket);
	sendEvent(QJsonObject{ { "type", "gameStart" }, { "game", QJsonObject{ { "gameId", challengeId } } } });
}

void FakeLichessServer::openGameStream(QTcpSocket* socket, const QString& gameId)
{
	auto game = _games.find(gameId);
	if (game == _games.end())
		return FakeLichess::answer(socket, 404, "Not Found");
	if (QDateTime::currentMSecsSinceEpoch() < _streamsDownUntil)
		return FakeLichess::answer(socket, 503, "Service Unavailable");
	FakeLichess::startStream(socket);
	game->streams << socket;
	FakeLichess::line(socket, gameFull(*game));
	emit streamsChanged();
}

void FakeLichessServer::boardAction(QTcpSocket* socket, const QString& gameId, const QStringList& action)
{
	auto game = _games.find(gameId);
	if (game == _games.end() || game->status != QLatin1String("started"))
		return FakeLichess::answer(socket, 400, "Bad Request");
	const QString what = action.value(0);
	const QString arg = action.value(1);
	const bool white = playerColor(*game) == QLatin1String("white");
	if (what == QLatin1String("move")) {
		game->moves << arg;
		game->wdraw = game->bdraw = false;
	} else if (what == QLatin1String("draw")) {
		const bool offered = white ? game->bdraw : game->wdraw;
		if (arg == QLatin1String("yes") && offered) {
			game->status = QStringLiteral("draw");
		} else if (arg == QLatin1String("yes")) {
			(white ? game->wdraw : game->bdraw) = true; // the player's own offer
		} else {
			game->wdraw = game->bdraw = false;
		}
	} else if (what == QLatin1String("resign")) {
		game->status = QStringLiteral("resign");
		game->winner = white ? QStringLiteral("black") : QStringLiteral("white");
	} else if (what != QLatin1String("takeback") && what != QLatin1String("abort") && what != QLatin1String("claim-victory")) {
		return FakeLichess::answer(socket, 404, "Not Found");
	}
	ok(socket);
	sendState(*game);
}

QByteArray FakeLichessServer::account() const
{
	return compact(QJsonObject{
		{ "id", username().toLower() },
		{ "username", username() },
		{ "perfs", QJsonObject{ { "rapid", QJsonObject{ { "rating", 1650 }, { "games", 42 }, { "prov", false } } } } },
	});
}

// /api/account/playing: the games not over, the player's side of each.
QByteArray FakeLichessServer::playing() const
{
	QJsonArray nowPlaying;
	for (const Game& game : _games) {
		if (game.status != QLatin1String("started"))
			continue;
		const bool white = playerColor(game) == QLatin1String("white");
		const QString opponent = white ? game.blackName : game.whiteName;
		nowPlaying.append(QJsonObject{
			{ "gameId", game.id },
			{ "fullId", game.id + "abcd" },
			{ "color", white ? "white" : "black" },
			{ "fen", game.fen },
			{ "opponent", QJsonObject{ { "id", opponent.toLower() }, { "username", opponent }, { "rating", 1712 } } },
			{ "isMyTurn", (game.moves.size() % 2 == 0) == white },
			{ "secondsLeft", 600 },
			{ "speed", "rapid" },
			{ "source", "friend" },
			{ "variant", QJsonObject{ { "key", "standard" }, { "name", "Standard" } } },
			{ "lastMove", game.moves.value(game.moves.size() - 1) },
			{ "rated", false },
			{ "perf", "rapid" },
		});
	}
	return compact(QJsonObject{ { "nowPlaying", nowPlaying } });
}

QJsonObject FakeLichessServer::player(const QString& name) const
{
	return QJsonObject{ { "id", name.toLower() }, { "name", name } };
}

QByteArray FakeLichessServer::gameFull(const Game& game) const
{
	return compact(QJsonObject{
		{ "type", "gameFull" },
		{ "id", game.id },
		{ "rated", false },
		{ "variant", QJsonObject{ { "key", "standard" } } },
		{ "speed", "rapid" },
		{ "clock", QJsonObject{ { "initial", 600000 }, { "increment", 5000 } } },
		{ "white", player(game.whiteName) },
		{ "black", player(game.blackName) },
		{ "initialFen", "startpos" },
		{ "state", gameState(game) },
	});
}

// The clocks stay at 10 minutes: no test is about Lichess's clock.
QJsonObject FakeLichessServer::gameState(const Game& game) const
{
	QJsonObject state{
		{ "type", "gameState" },
		{ "moves", game.moves.join(QLatin1Char(' ')) },
		{ "wtime", 600000 }, { "btime", 600000 }, { "winc", 5000 }, { "binc", 5000 },
		{ "status", game.status },
		{ "wdraw", game.wdraw }, { "bdraw", game.bdraw },
	};
	if (!game.winner.isEmpty())
		state.insert("winner", game.winner);
	return state;
}

void FakeLichessServer::sendState(Game& game)
{
	const QByteArray line = compact(gameState(game));
	for (const auto& socket : game.streams)
		if (socket && socket->state() == QAbstractSocket::ConnectedState)
			FakeLichess::line(socket, line);
	if (game.status != QLatin1String("started")) { // over: Lichess closes the stream
		for (const auto& socket : game.streams)
			if (socket)
				socket->disconnectFromHost();
		game.streams.clear();
	}
}

void FakeLichessServer::sendEvent(const QJsonObject& event)
{
	const QByteArray line = compact(event);
	for (const auto& socket : _eventStreams)
		if (socket && socket->state() == QAbstractSocket::ConnectedState)
			FakeLichess::line(socket, line);
}

QString FakeLichessServer::playerColor(const Game& game) const
{
	return game.whiteName == username() ? QStringLiteral("white") : QStringLiteral("black");
}

void FakeLichessServer::keepAlive()
{
	auto ping = [](QList<QPointer<QTcpSocket>>& sockets) {
		sockets.removeIf([](const QPointer<QTcpSocket>& s) { return !s || s->state() != QAbstractSocket::ConnectedState; });
		for (const auto& socket : sockets)
			FakeLichess::line(socket, QByteArray());
	};
	ping(_eventStreams);
	for (Game& game : _games)
		ping(game.streams);
}
