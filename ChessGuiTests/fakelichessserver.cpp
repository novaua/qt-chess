#include "fakelichessserver.h"

#include <QBuffer>
#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImage>
#include <QTcpSocket>
#include <QUrlQuery>

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
	_players.insert(username(), Player{ {}, 1650, false });
}

void FakeLichessServer::setPlayer(const QString& name, const QString& title, int rating, bool provisional,
                                  const QString& flair)
{
	_players.insert(name, Player{ title, rating, provisional, flair });
}

bool FakeLichessServer::eventStreamOpen() const
{
	for (const auto& socket : _eventStreams)
		if (socket && socket->state() == QAbstractSocket::ConnectedState)
			return true;
	return false;
}

bool FakeLichessServer::seekOpen() const
{
	return _seek && _seek->state() == QAbstractSocket::ConnectedState;
}

// ── What a test does as the other side ─────────────────────────────────

void FakeLichessServer::challenge(const QString& id, const QString& challenger, int rating, const QString& challengerColor,
                                  const QString& variant, const QString& startFen)
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
			{ "variant", QJsonObject{ { "key", variant } } },
			{ "initialFen", startFen.isEmpty() ? QStringLiteral("startpos") : startFen },
			{ "rated", false },
			{ "speed", "rapid" },
			{ "timeControl", QJsonObject{ { "type", "clock" }, { "limit", 600 }, { "increment", 5 }, { "show", "10+5" } } },
			{ "color", challengerColor },
		} },
	};
	if (!_players.contains(challenger))
		_players.insert(challenger, Player{ {}, rating, false });
	if (!_challenges.contains(id))
		_challengeOrder << id;
	_challenges.insert(id, event);
	sendEvent(event);
}

void FakeLichessServer::addOngoingGame(const QString& id, const QString& opponent, const QString& playerColor,
                                       const QString& moves, const QString& fen, const QString& variant,
                                       const QString& startFen)
{
	Game game;
	game.id = id;
	game.variant = variant;
	if (!startFen.isEmpty())
		game.initialFen = startFen;
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

bool FakeLichessServer::pairSeek(const QString& gameId, const QString& opponent)
{
	if (!seekOpen())
		return false;
	Game game;
	game.id = gameId;
	game.whiteName = username();
	game.blackName = opponent;
	_games.insert(gameId, game);
	// The game as Lichess makes it from the seek: its clock's speed, variant and rating.
	const QUrlQuery seek(bodyOf(QStringLiteral("POST /api/board/seek")));
	const double minutes = seek.queryItemValue(QStringLiteral("time")).toDouble();
	const int estimate = int(minutes * 60) + 40 * seek.queryItemValue(QStringLiteral("increment")).toInt();
	const QString speed = estimate < 1500 ? QStringLiteral("rapid") : QStringLiteral("classical");
	sendEvent(QJsonObject{ { "type", "gameStart" }, { "game", QJsonObject{
		{ "gameId", gameId }, { "fullId", gameId + "abcd" }, { "color", "white" }, { "source", "lobby" },
		{ "speed", speed }, { "rated", seek.queryItemValue(QStringLiteral("rated")) == QLatin1String("true") },
		{ "hasMoved", false },
		{ "variant", QJsonObject{ { "key", seek.queryItemValue(QStringLiteral("variant")) } } },
		{ "opponent", QJsonObject{ { "id", opponent.toLower() }, { "username", opponent } } } } } });
	FakeLichess::close(_seek); // taken: Lichess ends the seek's request
	return true;
}

void FakeLichessServer::refuseSeeks(int status, const QString& message)
{
	_seekRefusal = status;
	_seekRefusalMessage = message;
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
	// LI-09: /api/board/seek: a correspondence seek is answered at once, a real-time one held open
	// (keep-alive lines) until pairSeek() or the app closes it.
	if (!get && path == QLatin1String("/api/board/seek")) {
		if (_seekRefusal != 0)
			return FakeLichess::json(socket, compact(QJsonObject{ { "error", _seekRefusalMessage } }), _seekRefusal);
		if (_server.bodies.last().contains(QLatin1String("days=")))
			return FakeLichess::json(socket, R"({"id":"corrSeek"})");
		FakeLichess::startStream(socket);
		_seek = socket;
		connect(socket, &QTcpSocket::disconnected, this, &FakeLichessServer::streamsChanged);
		emit streamsChanged();
		return;
	}
	// LI-11: POST /api/users (comma-separated ids): the users, with their flairs.
	if (!get && path == QLatin1String("/api/users")) {
		QJsonArray users;
		for (const QString& id : _server.bodies.last().split(QLatin1Char(','), Qt::SkipEmptyParts))
			for (auto it = _players.cbegin(); it != _players.cend(); ++it)
				if (it.key().toLower() == id.trimmed()) {
					QJsonObject user{ { "id", id.trimmed() }, { "username", it.key() } };
					if (!it->flair.isEmpty())
						user.insert("flair", it->flair);
					users.append(user);
				}
		return FakeLichess::json(socket, QJsonDocument(users).toJson(QJsonDocument::Compact));
	}
	// LI-11: a flair's image, as lichess1.org serves it: a small WebP (which needs Qt's WebP plugin, here too).
	if (get && path.startsWith(QLatin1String("/assets/flair/img/")) && path.endsWith(QLatin1String(".webp"))) {
		QImage image(16, 16, QImage::Format_ARGB32);
		image.fill(QColor(0xE0, 0x40, 0x40));
		QByteArray webp;
		QBuffer buffer(&webp);
		buffer.open(QIODevice::WriteOnly);
		if (!image.save(&buffer, "WEBP"))
			qWarning() << "FakeLichess: no WebP plugin: the flair can't be made";
		return FakeLichess::bytes(socket, webp, "image/webp");
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
	game.variant = challenge.value("variant").toObject().value("key").toString();
	game.initialFen = challenge.value("initialFen").toString();
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
	} else if (what == QLatin1String("abort")) {
		game->status = QStringLiteral("aborted"); // no winner
	} else if (what == QLatin1String("claim-victory")) {
		game->status = QStringLiteral("timeout"); // as Lichess ends a claimed game: the player wins
		game->winner = playerColor(*game);
	} else if (what != QLatin1String("takeback")) {
		return FakeLichess::answer(socket, 404, "Not Found");
	}
	// takeback: the player's request waits for the opponent, who never answers here (and never asks).
	ok(socket);
	sendState(*game);
}

QByteArray FakeLichessServer::account() const
{
	return compact(QJsonObject{
		{ "id", username().toLower() },
		{ "username", username() },
		{ "perfs", QJsonObject{ { "rapid", QJsonObject{ { "rating", 1650 }, { "games", 42 }, { "prov", false } } },
		                         { "chess960", QJsonObject{ { "rating", 1480 }, { "games", 7 }, { "prov", true } } } } },
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
			{ "variant", QJsonObject{ { "key", game.variant } } },
			{ "lastMove", game.moves.value(game.moves.size() - 1) },
			{ "rated", false },
			{ "perf", "rapid" },
		});
	}
	return compact(QJsonObject{ { "nowPlaying", nowPlaying } });
}

// As gameFull's players (GameEventPlayer): with a title, rating and provisional when set (LI-07).
QJsonObject FakeLichessServer::player(const QString& name) const
{
	QJsonObject p{ { "id", name.toLower() }, { "name", name } };
	const auto it = _players.constFind(name);
	if (it == _players.constEnd())
		return p;
	p.insert("title", it->title.isEmpty() ? QJsonValue() : QJsonValue(it->title)); // null, as Lichess sends it
	if (it->rating > 0)
		p.insert("rating", it->rating);
	p.insert("provisional", it->provisional);
	return p;
}

QByteArray FakeLichessServer::gameFull(const Game& game) const
{
	return compact(QJsonObject{
		{ "type", "gameFull" },
		{ "id", game.id },
		{ "rated", false },
		{ "variant", QJsonObject{ { "key", game.variant } } },
		{ "speed", "rapid" },
		{ "clock", QJsonObject{ { "initial", 600000 }, { "increment", 5000 } } },
		{ "white", player(game.whiteName) },
		{ "black", player(game.blackName) },
		{ "initialFen", game.initialFen },
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
	if (seekOpen())
		FakeLichess::line(_seek, QByteArray());
	for (Game& game : _games)
		ping(game.streams);
}
