#include "stdafx.h"
#include "LichessClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QRegularExpression>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "Crypt32.lib")
#endif

static QString apiErrorMessage(const QByteArray& body, const QNetworkReply* reply); // below

LichessClient::LichessClient(QObject* parent)
    : QObject(parent)
{
    // Prevent Qt from intercepting 401 responses and overriding our Bearer token.
    connect(&_nam, &QNetworkAccessManager::authenticationRequired,
            this, [this](QNetworkReply* reply, QAuthenticator*) { checkAuthExpired(reply, true); reply->abort(); });
    connect(&_nam, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply* reply) { checkAuthExpired(reply, false); });

    _gameStream = new LichessStream(_nam, QStringLiteral("stream"), this);
    connect(_gameStream, &LichessStream::line, this, [this](const QByteArray& line) { handleGameStreamLine(line); });
    connect(_gameStream, &LichessStream::connectionChanged, this, &LichessClient::gameConnectionChanged);
    connect(_gameStream, &LichessStream::stopped, this, &LichessClient::gameStreamRefused);
    connect(_gameStream, &LichessStream::ended, this, &LichessClient::gameStreamEnded);
}

void LichessClient::setStreamTiming(int silenceMs, int firstRetryMs, int moveTimeoutMs)
{
    _gameStream->setTiming(silenceMs, firstRetryMs);
    _moveTimeoutMs = moveTimeoutMs;
}

// Any call made with our token that Lichess answers with 401: the token was revoked or expired.
// validateToken() sends a different token, so it doesn't count.
// Both authenticationRequired and finished may see the same 401: reported once per reply.
void LichessClient::checkAuthExpired(QNetworkReply* reply, bool unauthorized)
{
    if (_token.isEmpty() || reply->property("authExpiredReported").toBool()
        || reply->request().rawHeader("Authorization") != ("Bearer " + _token).toUtf8())
        return;
    if (unauthorized || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
        reply->setProperty("authExpiredReported", true);
        qDebug() << "LichessClient: 401 for" << reply->url().path() << "- the token is no longer valid";
        emit authExpired();
    }
}

void LichessClient::refreshAccount()
{
    if (_token.isEmpty() || _offline) return;
    auto* reply = _nam.get(makeRequest(QStringLiteral("/api/account")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return; // a 401 is reported by checkAuthExpired
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;
        const QString username = doc.object().value(QStringLiteral("username")).toString();
        if (!username.isEmpty())
            emit accountFetched(username, parseRatings(doc.object()));
    });
}

QVariantMap LichessClient::parseRatings(const QJsonObject& account)
{
    QVariantMap ratings;
    const QJsonObject perfs = account.value(QStringLiteral("perfs")).toObject();
    for (const char* pool : { "bullet", "blitz", "rapid", "classical", "correspondence" }) {
        const QJsonObject perf = perfs.value(QLatin1String(pool)).toObject();
        if (!perf.contains(QStringLiteral("rating")))
            continue;
        ratings.insert(QLatin1String(pool), QVariantMap{
            { QStringLiteral("rating"), perf.value(QStringLiteral("rating")).toInt() },
            { QStringLiteral("games"),  perf.value(QStringLiteral("games")).toInt() },
            { QStringLiteral("prov"),   perf.value(QStringLiteral("prov")).toBool() }, // provisional: few games
        });
    }
    return ratings;
}

void LichessClient::setToken(const QString& decryptedToken)
{
    const bool changed = _token != decryptedToken;
    _token = decryptedToken;
    qDebug() << "LichessClient: token" << (_token.isEmpty() ? "cleared" : QString("set(%1 chars)").arg(_token.size()));
    emit canRematchChanged();
    if (changed)
        setOngoingGames({}); // another account (or none): its games are not ours
}

void LichessClient::cancelOngoingGamesRefresh()
{
    if (_ongoingGamesReply) {
        auto* old = _ongoingGamesReply;
        _ongoingGamesReply = nullptr; // before abort(): its finished handler runs inside abort()
        old->abort();
    }
}

void LichessClient::refreshOngoingGames()
{
    if (_token.isEmpty() || _offline) return;
    cancelOngoingGamesRefresh();
    _ongoingGamesLoading = true;
    _ongoingGamesError.clear();
    emit ongoingGamesChanged();

    auto* reply = _nam.get(makeRequest(QStringLiteral("/api/account/playing?nb=50")));
    _ongoingGamesReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply != _ongoingGamesReply)
            return; // replaced by a newer refresh, or the account changed
        _ongoingGamesReply = nullptr;
        _ongoingGamesLoading = false;

        const QByteArray raw = reply->readAll();
        const auto doc = QJsonDocument::fromJson(raw);
        if (reply->error() != QNetworkReply::NoError || !doc.isObject()) {
            // The list is kept. A 401 (the handler for Lichess's auth challenge aborts the reply,
            // hence "canceled") is reported by checkAuthExpired: the profile's token is then
            // dropped, which clears the list (setToken).
            const bool unauthorized = reply->property("authExpiredReported").toBool()
                || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401;
            _ongoingGamesError = unauthorized ? QStringLiteral("Lichess sign-in expired")
                : reply->error() != QNetworkReply::NoError ? apiErrorMessage(raw, reply)
                : QStringLiteral("Invalid response");
            qDebug() << "LichessClient: ongoing games failed:" << _ongoingGamesError;
        } else {
            _ongoingGames = parseOngoingGames(doc.object());
            _ongoingGamesError.clear();
            qDebug() << "LichessClient: ongoing games:" << _ongoingGames.size();
            for (const auto& game : _ongoingGames) {
                const auto row = game.toMap();
                qDebug() << "  " << row.value("gameId").toString() << row.value("speed").toString()
                         << row.value("source").toString() << row.value("variant").toString()
                         << "vs" << row.value("opponentName").toString()
                         << (row.value("isMyTurn").toBool() ? "my turn" : "their turn")
                         << row.value("timeLeftText").toString()
                         << (row.value("canPlayHere").toBool() ? "" : "(not playable here)");
            }
        }
        emit ongoingGamesChanged();
    });
}

void LichessClient::setOngoingGames(const QVariantList& games)
{
    cancelOngoingGamesRefresh(); // its answer would replace these
    _ongoingGames = games;
    _ongoingGamesLoading = false;
    _ongoingGamesError.clear();
    emit ongoingGamesChanged();
}

// Like "Join a game", it replaces whatever stream or challenge wait is running: it is offered only
// outside a game (the Online page).
void LichessClient::rejoinGame(const QString& gameId)
{
    if (gameId.isEmpty())
        return;
    qDebug() << "LichessClient: rejoin" << gameId;
    streamGame(gameId);
}

// static
// Rounded down, like a clock, but never by more than about half: hours up to two days, seconds up to
// two minutes ("47 h", not "1 day"; "119 s", not "1 min").
QString LichessClient::timeLeftText(int seconds)
{
    if (seconds < 0)
        return {};
    if (seconds < 120)
        return QStringLiteral("%1 s").arg(seconds);
    if (seconds < 2 * 3600)
        return QStringLiteral("%1 min").arg(seconds / 60);
    if (seconds < 2 * 86400)
        return QStringLiteral("%1 h").arg(seconds / 3600);
    return QStringLiteral("%1 days").arg(seconds / 86400);
}

// The Board API plays Rapid, Classical and Correspondence; Blitz only in a direct challenge (from
// the website "friend", from the API "api"), bulk pairing ("api") or vs Lichess AI (Lichess API spec,
// "Board → Restrictions"). Only the standard variant here (LI-01).
static bool playableHere(const QString& speed, const QString& source, const QString& variant)
{
    if (variant != QLatin1String("standard"))
        return false;
    if (speed == QLatin1String("rapid") || speed == QLatin1String("classical") || speed == QLatin1String("correspondence"))
        return true;
    return speed == QLatin1String("blitz")
        && (source == QLatin1String("friend") || source == QLatin1String("api") || source == QLatin1String("ai"));
}

// static
QVariantList LichessClient::parseOngoingGames(const QJsonObject& playing)
{
    QVariantList games;
    for (const auto& item : playing.value(QStringLiteral("nowPlaying")).toArray()) {
        const QJsonObject game = item.toObject();
        const QString gameId = game.value(QStringLiteral("gameId")).toString();
        if (gameId.isEmpty())
            continue;

        const QJsonObject opponent = game.value(QStringLiteral("opponent")).toObject();
        const bool isAi = opponent.contains(QStringLiteral("ai"));
        QString opponentName = opponent.value(QStringLiteral("username")).toString();
        if (isAi)
            opponentName = QStringLiteral("Lichess AI level %1").arg(opponent.value(QStringLiteral("ai")).toInt());
        else if (opponentName.isEmpty())
            opponentName = QStringLiteral("Anonymous");

        // "variant" is { "key": "standard", ... }; a plain string is accepted too.
        const QJsonValue variantValue = game.value(QStringLiteral("variant"));
        QString variant = variantValue.isObject() ? variantValue.toObject().value(QStringLiteral("key")).toString()
                                                  : variantValue.toString();
        if (variant.isEmpty())
            variant = QStringLiteral("standard");

        const QJsonValue secondsValue = game.value(QStringLiteral("secondsLeft"));
        const int secondsLeft = secondsValue.isDouble() ? secondsValue.toInt() : -1; // none: no clock
        const QString speed  = game.value(QStringLiteral("speed")).toString();
        const QString source = game.value(QStringLiteral("source")).toString();

        games.append(QVariantMap{
            { QStringLiteral("gameId"),         gameId },
            { QStringLiteral("color"),          game.value(QStringLiteral("color")).toString() },
            { QStringLiteral("opponentName"),   opponentName },
            { QStringLiteral("opponentRating"), opponent.value(QStringLiteral("rating")).toInt() }, // 0: none
            { QStringLiteral("opponentIsAi"),   isAi },
            { QStringLiteral("isMyTurn"),       game.value(QStringLiteral("isMyTurn")).toBool() },
            { QStringLiteral("secondsLeft"),    secondsLeft },
            { QStringLiteral("timeLeftText"),   timeLeftText(secondsLeft) },
            { QStringLiteral("speed"),          speed },
            { QStringLiteral("source"),         source },
            { QStringLiteral("variant"),        variant },
            { QStringLiteral("canPlayHere"),    playableHere(speed, source, variant) },
            { QStringLiteral("lastMove"),       game.value(QStringLiteral("lastMove")).toString() },
            { QStringLiteral("fen"),            game.value(QStringLiteral("fen")).toString() },
            { QStringLiteral("rated"),          game.value(QStringLiteral("rated")).toBool() },
        });
    }
    return games;
}

void LichessClient::setUsername(const QString& username)
{
    _username = username.toLower(); // Lichess IDs are always lowercase
}

QNetworkRequest LichessClient::makeRequest(const QString& path) const
{
    qDebug() << "LichessClient: >>" << path
             << "| token:" << (_token.isEmpty() ? "MISSING" : QString("set(%1 chars)").arg(_token.size()));
    QNetworkRequest req(QUrl(_baseUrl + path));
    if (!_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + _token).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    return req;
}

void LichessClient::validateToken(const QString& token)
{
    QNetworkRequest req(QUrl(_baseUrl + "/api/account"));
    req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    auto* reply = _nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, token]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit tokenValidated(false, {});
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull() || !doc.isObject()) {
            emit tokenValidated(false, {});
            return;
        }
        const QString username = doc.object().value(QStringLiteral("username")).toString();
        emit tokenValidated(!username.isEmpty(), username);
    });
}

// Lichess error bodies are {"error":"..."}; fall back to Qt's message.
static QString apiErrorMessage(const QByteArray& body, const QNetworkReply* reply)
{
    const auto doc = QJsonDocument::fromJson(body);
    const QString apiError = doc.isObject()
        ? doc.object().value(QStringLiteral("error")).toString() : QString();
    return apiError.isEmpty() ? reply->errorString() : apiError;
}

// Real-time clock, correspondence days, or neither (unlimited).
static void addTimeControl(QUrlQuery& body, int limitSec, int incrementSec, int days)
{
    if (limitSec > 0 || incrementSec > 0) {
        body.addQueryItem(QStringLiteral("clock.limit"),     QString::number(limitSec));
        body.addQueryItem(QStringLiteral("clock.increment"), QString::number(incrementSec));
    }
    else if (days > 0) {
        body.addQueryItem(QStringLiteral("days"), QString::number(days));
    }
}

void LichessClient::createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant,
                                        int days)
{
    createOpenChallengeSec(minutes * 60, minutes > 0 ? increment : 0, minutes > 0 ? 0 : days, color, variant);
}

void LichessClient::createOpenChallengeSec(int limitSec, int incrementSec, int days,
                                           const QString& color, const QString& variant)
{
    QUrlQuery body;
    addTimeControl(body, limitSec, incrementSec, days);
    // No "color" here: /api/challenge/open has none (Lichess answers "color":"random" and picks a
    // finalColor). Our side is taken when we accept it below.
    body.addQueryItem(QStringLiteral("variant"), variant);

    if (_openChallengeReply) {
        auto* previous = _openChallengeReply;
        _openChallengeReply = nullptr;
        previous->abort();
    }
    auto* reply = _nam.post(makeRequest(QStringLiteral("/api/challenge/open")),
                            body.toString(QUrl::FullyEncoded).toUtf8());
    // Kept so stopStream() can drop a request still in flight (e.g. the user left the game).
    _openChallengeReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, color]() {
        reply->deleteLater();
        if (reply != _openChallengeReply)
            return; // aborted by stopStream() or replaced
        _openChallengeReply = nullptr;

        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "LichessClient: createOpenChallenge HTTP" << httpStatus << raw.left(500);

        if (reply->error() != QNetworkReply::NoError) {
            emit networkError(apiErrorMessage(raw, reply));
            return;
        }

        const auto doc = QJsonDocument::fromJson(raw);
        if (doc.isNull() || !doc.isObject()) { emit networkError(QStringLiteral("Invalid response")); return; }

        const QJsonObject root = doc.object();

        // Lichess returns application-level errors as {"error":"..."} with HTTP 200
        const QString apiError = root.value(QStringLiteral("error")).toString();
        if (!apiError.isEmpty()) { emit networkError(apiError); return; }

        const QString gameId = root.value(QStringLiteral("id")).toString();
        if (gameId.isEmpty()) { emit networkError(QStringLiteral("No game ID in response")); return; }

        // All URL fields are at the root level of the response.
        // Give the friend the URL for their side:
        //   creator=white → friend plays black → urlBlack
        //   creator=black → friend plays white → urlWhite
        //   random        → base url (random assignment)
        QString joinUrl;
        if (color == QLatin1String("white"))
            joinUrl = root.value(QStringLiteral("urlBlack")).toString();
        else if (color == QLatin1String("black"))
            joinUrl = root.value(QStringLiteral("urlWhite")).toString();
        if (joinUrl.isEmpty())
            joinUrl = root.value(QStringLiteral("url")).toString();

        _currentGameId = gameId;
        emit challengeCreated(gameId, joinUrl);
        waitForGameStart(gameId, true, color);
    });
}

void LichessClient::cancelChallenge(const QString& challengeId)
{
    stopStream();
    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/challenge/") + challengeId + QStringLiteral("/cancel")),
        QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (_currentGameId.isEmpty()) // else the player has started something new meanwhile
            emit challengeCanceled();
    });
}

// The accept URL; for an open challenge "?color=" takes that side (only white or black are valid).
static QString acceptPath(const QString& challengeId, const QString& color)
{
    QString path = QStringLiteral("/api/challenge/") + challengeId + QStringLiteral("/accept");
    if (color == QLatin1String("white") || color == QLatin1String("black"))
        path += QStringLiteral("?color=") + color;
    return path;
}

bool LichessClient::joinFromUrl(const QString& urlOrId)
{
    const QString gameId = gameIdFromUrl(urlOrId);
    if (gameId.isEmpty())
        return false;
    acceptChallenge(gameId, colorFromUrl(urlOrId));
    return true;
}

void LichessClient::resumeOpenChallenge(const QString& gameId, const QString& friendUrl)
{
    const QString friendColor = colorFromUrl(friendUrl);
    const QString myColor = friendColor == QLatin1String("white") ? QStringLiteral("black")
                          : friendColor == QLatin1String("black") ? QStringLiteral("white")
                          : QString();
    waitForGameStart(gameId, true, myColor);
}

void LichessClient::acceptChallenge(const QString& challengeId, const QString& color)
{
    if (_acceptReply) {
        auto* previous = _acceptReply;
        _acceptReply = nullptr;
        previous->abort();
    }
    auto* reply = _nam.post(makeRequest(acceptPath(challengeId, color)), QByteArray());
    // Kept so stopStream() can drop it (Back on the Online page before Lichess answered).
    _acceptReply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, challengeId]() {
        reply->deleteLater();
        if (reply != _acceptReply)
            return; // stopped or replaced: no game must start
        _acceptReply = nullptr;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        qDebug() << "LichessClient: accept" << challengeId
                 << "| HTTP" << status << "| error:" << reply->error()
                 << "| body:" << body.left(200);
        // A 404 is fine: the game has started already (a rejoin by id), the stream follows.
        // Anything else that failed: Lichess's own reason (e.g. a challenge for someone else).
        if (reply->property("authExpiredReported").toBool() || status == 401) {
            emit networkError(QStringLiteral("Lichess sign-in expired"));
            return;
        }
        if (reply->error() != QNetworkReply::NoError && status != 404) {
            emit networkError(apiErrorMessage(body, reply));
            return;
        }
        streamGame(challengeId);
    });
}

void LichessClient::waitForGameStart(const QString& gameId, bool registerAsPlayer, const QString& myColor)
{
    stopEventStream();
    _pollTimer.stop();
    _waitForGameId = gameId;

    // Must call accept to register the creator as a Board API player; without it
    // the board-game stream endpoint returns 404 even after the opponent joins.
    // It also picks our side: without a colour Lichess gives us its random finalColor.
    if (registerAsPlayer) {
        auto* reg = _nam.post(makeRequest(acceptPath(gameId, myColor)), QByteArray());
        connect(reg, &QNetworkReply::finished, this, [reg]() { reg->deleteLater(); });
    }

    openEventStream();

    connect(&_pollTimer, &QTimer::timeout, this, &LichessClient::checkGameReady,
            Qt::UniqueConnection);
    _pollTimer.start(3000);
}

void LichessClient::openEventStream()
{
    QNetworkRequest req = makeRequest(QStringLiteral("/api/stream/event"));
    req.setRawHeader("Accept", "application/x-ndjson");
    _eventStreamReply = _nam.get(req);
    // Capture the reply by value to avoid dereferencing _eventStreamReply after
    // stopEventStream() nulls it (errorOccurred may fire asynchronously).
    auto* evReply = _eventStreamReply;
    connect(evReply, &QNetworkReply::readyRead, this, [this, evReply]() {
        if (evReply == _eventStreamReply)
            handleEventStreamData(evReply);
    });
    connect(evReply, &QNetworkReply::errorOccurred, this,
            [evReply](QNetworkReply::NetworkError code) {
                const int status = evReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << "LichessClient: event stream error | HTTP" << status
                         << "| code:" << code << "|" << evReply->errorString();
            });
}

void LichessClient::handleEventStreamData(QNetworkReply* reply)
{
    while (reply->canReadLine()) {
        const QByteArray line = reply->readLine().trimmed();
        qDebug() << "LichessClient: event stream line:" << line.left(300);
        if (line.isEmpty()) continue;

        const auto doc = QJsonDocument::fromJson(line);
        if (doc.isNull() || !doc.isObject()) continue;

        const QJsonObject obj  = doc.object();
        const QString     type = obj.value(QStringLiteral("type")).toString();

        if (type == QLatin1String("gameStart")) {
            const QJsonObject game   = obj.value(QStringLiteral("game")).toObject();
            const QString     gameId = game.value(QStringLiteral("id")).toString();
            qDebug() << "LichessClient: event stream gameStart" << gameId
                     << "| waiting for:" << _waitForGameId;
            if (_waitForGameId.isEmpty() || gameId != _waitForGameId)
                continue; // only watching for a rematch offer, or another game
            onGameStartDetected(gameId);
            return;
        }
        if (type == QLatin1String("challenge") || type == QLatin1String("challengeCanceled"))
            handleRematchOffer(type, obj.value(QStringLiteral("challenge")).toObject());
    }
}

void LichessClient::checkGameReady()
{
    if (_waitForGameId.isEmpty()) {
        _pollTimer.stop();
        return;
    }

    // Abort any still-in-flight probe before launching a new one.
    stopProbe();

    qDebug() << "LichessClient: polling stream" << _waitForGameId;

    QNetworkRequest req = makeRequest(
        QStringLiteral("/api/board/game/stream/") + _waitForGameId);
    req.setRawHeader("Accept", "application/x-ndjson");

    _probeReply = _nam.get(req);
    auto* probe = _probeReply;

    connect(probe, &QNetworkReply::metaDataChanged, this, [this, probe]() {
        if (probe != _probeReply) return; // stale
        const int status = probe->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 0) return;
        qDebug() << "LichessClient: stream probe HTTP" << status;
        _probeReply = nullptr;
        probe->abort();
        probe->deleteLater();
        if (status == 200 && !_waitForGameId.isEmpty()) {
            const QString id = _waitForGameId;
            onGameStartDetected(id);
        }
    });

    connect(probe, &QNetworkReply::errorOccurred, this,
            [this, probe](QNetworkReply::NetworkError code) {
                qDebug() << "LichessClient: stream probe error" << code;
                if (_probeReply == probe) _probeReply = nullptr;
                probe->deleteLater();
            });
}

void LichessClient::stopProbe()
{
    if (_probeReply) {
        auto* probe = _probeReply;
        _probeReply = nullptr; // before abort(): its errorOccurred handler runs inside abort()
        probe->abort();
        probe->deleteLater();
    }
}

void LichessClient::onGameStartDetected(const QString& gameId)
{
    _pollTimer.stop();
    stopEventStream();
    streamGame(gameId);
}

void LichessClient::stopEventStream()
{
    if (_eventStreamReply) {
        _eventStreamReply->abort();
        _eventStreamReply->deleteLater();
        _eventStreamReply = nullptr;
    }
    _waitForGameId.clear();
}

void LichessClient::streamGame(const QString& gameId)
{
    stopStream();
    _currentGameId   = gameId;
    _lastMovesList.clear();
    _sentMoves.clear();
    _lastOppDraw     = false;
    _lastOppTakeback = false;
    _gameFullSeen    = false;
    QNetworkRequest req = makeRequest(QStringLiteral("/api/board/game/stream/") + gameId);
    req.setRawHeader("Accept", "application/x-ndjson");
    _gameStream->open(req); // established (reconnects) once its gameFull has come
}

// Ended without a game: whoever waits for gameStarted (the Online page) must hear about it. Not
// when stopStream() aborted it (canceled).
void LichessClient::gameStreamEnded(QNetworkReply::NetworkError error, const QString& errorString, bool authExpired)
{
    if (authExpired)
        emit networkError(QStringLiteral("Lichess sign-in expired"));
    else if (error == QNetworkReply::NoError)
        emit networkError(QStringLiteral("Lichess closed the game before it started"));
    else if (error != QNetworkReply::OperationCanceledError)
        emit networkError(errorString);
}

// Mid-game, Lichess's own "no" (4xx: the sign-in expired, a game it doesn't stream any more) won't
// change by asking again (LI-06).
void LichessClient::gameStreamRefused(bool authExpired, int httpStatus)
{
    const QString message = authExpired
        ? QStringLiteral("Lichess sign-in expired")
        : QStringLiteral("Lichess stopped sending this game (HTTP %1)").arg(httpStatus);
    qDebug() << "LichessClient: game stream not reconnected:" << message;
    emit gameStreamStopped();
    emit networkError(message);
}

void LichessClient::resyncGame()
{
    if (_currentGameId.isEmpty() || !_gameFullSeen)
        return;
    _gameStream->reopen(); // not while a reconnect is on its way anyway, nor after the game's end
}

bool LichessClient::handleGameStreamLine(const QByteArray& line)
{
    if (line.isEmpty()) return true;

    const auto doc = QJsonDocument::fromJson(line);
    if (doc.isNull() || !doc.isObject()) return true;

    const QJsonObject obj  = doc.object();
    const QString     type = obj.value(QStringLiteral("type")).toString();

    if (type == QLatin1String("gameFull")) {
        const bool resync = _gameFullSeen; // LI-06: the stream is back (or resyncGame): the same game
        _gameFullSeen = true;
        _gameStream->setReconnect(true);
        const QJsonObject white = obj.value(QStringLiteral("white")).toObject();
        const QJsonObject black = obj.value(QStringLiteral("black")).toObject();
        const QJsonObject state = obj.value(QStringLiteral("state")).toObject();

        const QString myColor  = obj.value(QStringLiteral("myColor")).toString();
        const QString whiteId  = white.value(QStringLiteral("id")).toString();
        const bool    isWhite  = resolveIsWhite(myColor, whiteId, _username);
        _playingAsWhite = isWhite;
        qDebug() << "LichessClient: playing as" << (isWhite ? "white" : "black")
                 << "| myColor=" << myColor << "| whiteId=" << whiteId << "| username=" << _username;

        const QStringList moves = state.value(QStringLiteral("moves")).toString()
                                      .split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (resync) {
            // The moves first, then "connected", then a lost move's message (it must stay on screen).
            const bool lost = resyncMoves(moves);
            _gameStream->connected();
            if (lost)
                emit networkError(QStringLiteral("Your move didn't reach Lichess. Play it again."));
        } else {
            _lastGame = parseRematchInfo(obj, _username);
            emit canRematchChanged();

            const QJsonObject opponent = isWhite ? black : white;
            const QString opponentName      = opponent.value(QStringLiteral("name")).toString();
            const QString opponentAvatarUrl = {}; // Lichess doesn't provide avatar in stream

            // Emit gameStarted first so the board is initialized before any moves are applied.
            _lastMovesList = moves;
            _sentMoves.clear();
            emit gameStarted(_currentGameId, isWhite, opponentName, opponentAvatarUrl);
            startClock(obj);

            // Resync our board state with moves that arrived before we joined.
            if (!_lastMovesList.isEmpty())
                emit opponentMoveReceived(_lastMovesList.join(QLatin1Char(' ')));
        }

        // Streaming a game that has already ended: Lichess sends only gameFull, then closes.
        const QString status = state.value(QStringLiteral("status")).toString();
        const bool ended = !status.isEmpty() && status != QLatin1String("started")
                && status != QLatin1String("created");
        updateClock(state, ended);
        if (ended) {
            _gameStream->close(); // the rest isn't read, and its end isn't a lost connection
            emit gameEnded(status, state.value(QStringLiteral("winner")).toString());
            return false;
        }

    } else if (type == QLatin1String("opponentGone")) {
        const bool gone = obj.value(QStringLiteral("gone")).toBool();
        const int  claimIn = obj.contains(QStringLiteral("claimWinInSeconds"))
            ? obj.value(QStringLiteral("claimWinInSeconds")).toInt() : -1;
        emit opponentGone(gone, claimIn);

    } else if (type == QLatin1String("gameState")) {
        const QString moves  = obj.value(QStringLiteral("moves")).toString();
        const QString status = obj.value(QStringLiteral("status")).toString();

        // The state that ends the game also carries its last move (e.g. the opponent's mate):
        // the move first, then the end.
        applyStreamMoves(moves.split(QLatin1Char(' '), Qt::SkipEmptyParts));

        const bool ended = status != QLatin1String("started") && !status.isEmpty();
        updateClock(obj, ended);
        if (ended) {
            _gameStream->close(); // the rest isn't read, and its end isn't a lost connection
            const QString winner = obj.value(QStringLiteral("winner")).toString();
            emit gameEnded(status, winner);
            return false;
        }

        const bool oppDraw = _playingAsWhite
            ? obj.value(QStringLiteral("bdraw")).toBool()
            : obj.value(QStringLiteral("wdraw")).toBool();
        const bool oppTakeback = _playingAsWhite
            ? obj.value(QStringLiteral("btakeback")).toBool()
            : obj.value(QStringLiteral("wtakeback")).toBool();
        if (oppDraw     && !_lastOppDraw)     emit drawOfferReceived();
        if (oppTakeback && !_lastOppTakeback) emit takebackRequested();
        _lastOppDraw     = oppDraw;
        _lastOppTakeback = oppTakeback;
    }
    return true;
}

void LichessClient::startClock(const QJsonObject& gameFull)
{
    const QJsonObject clock = gameFull.value(QStringLiteral("clock")).toObject();
    const int days = gameFull.value(QStringLiteral("daysPerTurn")).toInt();
    _correspondence = clock.isEmpty() && days > 0;
    _hasClock = _correspondence || !clock.isEmpty();
    if (_correspondence)
        emit clockStarted(days * 24 * 3600 * 1000LL, 0, true);
    else if (_hasClock)
        emit clockStarted(clock.value(QStringLiteral("initial")).toInteger(),
                          clock.value(QStringLiteral("increment")).toInteger(), false);
}

// Without a clock Lichess still sends wtime and btime: the largest int. A real-time clock runs once
// both sides have moved, a correspondence one from the start. A state without our sent move yet (e.g.
// an offer came first) is older than our board, where the clock has switched already.
void LichessClient::updateClock(const QJsonObject& state, bool gameOver)
{
    if (!_hasClock || !state.contains(QStringLiteral("wtime")) || (!gameOver && !_sentMoves.isEmpty()))
        return;
    const int plies = int(_lastMovesList.size());
    emit clockUpdated(state.value(QStringLiteral("wtime")).toInteger(), state.value(QStringLiteral("btime")).toInteger(),
                      plies % 2 == 0, !gameOver && (_correspondence || plies >= 2));
}

// The moves of a gameState: new ones are the opponent's, or the echo of ours (_sentMoves). A list
// that is shorter than ours (an accepted takeback) or differs from it replaces ours.
void LichessClient::applyStreamMoves(const QStringList& moves)
{
    const int seen = int(_lastMovesList.size());
    if (moves.size() < seen || moves.mid(0, seen) != _lastMovesList) {
        replaceMoveList(moves);
        return;
    }
    QStringList added = moves.mid(seen);
    while (!added.isEmpty() && !_sentMoves.isEmpty() && added.first() == _sentMoves.first()) {
        added.removeFirst();
        _sentMoves.removeFirst();
    }
    if (!added.isEmpty() && !_sentMoves.isEmpty()) { // Lichess has another move than the one we sent
        replaceMoveList(moves);
        return;
    }
    // A state without our sent move yet (e.g. an offer came first) just waits for its echo.
    _lastMovesList = moves;
    for (const auto& move : added)
        emit opponentMoveReceived(move);
}

// After a reconnect (or resyncGame): Lichess's moves against ours. Ours plus maybe the opponent's new
// ones: as for a gameState. Without a move we sent: if its POST is still on its way, Lichess may
// play it yet (its answer or the stream tells); otherwise it never got there: our board takes it
// back, and the player plays it again (returns true).
bool LichessClient::resyncMoves(const QStringList& moves)
{
    const QStringList ours = _lastMovesList + _sentMoves;
    if (moves.mid(0, ours.size()) == ours) {
        applyStreamMoves(moves);
        return false;
    }
    const bool missing = !_sentMoves.isEmpty() && moves == _lastMovesList;
    if (missing && std::any_of(_sentMoves.begin(), _sentMoves.end(),
                               [this](const QString& move) { return _postsInFlight.contains(move); })) {
        qDebug() << "LichessClient: resync before our move's POST has an answer: waiting for it";
        return false;
    }
    replaceMoveList(moves, missing);
    return missing;
}

void LichessClient::replaceMoveList(const QStringList& moves, bool moveRefused)
{
    qDebug() << "LichessClient: move list replaced; ours:" << (_lastMovesList + _sentMoves).join(QLatin1Char(' '))
             << "| Lichess:" << moves.join(QLatin1Char(' '));
    _lastMovesList = moves;
    _sentMoves.clear();
    emit moveListReplaced(moves.join(QLatin1Char(' ')), moveRefused);
}

void LichessClient::postMove(const QString& gameId, const QString& uciMove)
{
    trackSentMove(uciMove);
    if (_offline)
        return; // screenshot mode: the scene's stream says what Lichess made of it

    QNetworkRequest req = makeRequest(QStringLiteral("/api/board/game/") + gameId
                                      + QStringLiteral("/move/") + uciMove);
    req.setTransferTimeout(_moveTimeoutMs);
    auto* reply = _nam.post(req, QByteArray());
    _postsInFlight.append(uciMove);

    connect(reply, &QNetworkReply::finished, this, [this, reply, gameId, uciMove]() {
        reply->deleteLater();
        _postsInFlight.removeOne(uciMove);
        if (reply->error() == QNetworkReply::NoError)
            return;
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool pending = gameId == _currentGameId && _sentMoves.contains(uciMove);
        if (httpStatus < 400) {
            // No answer (a timeout, the connection gone), or none that says no: Lichess may have
            // played it or not. The stream anew tells (LI-06), and says so if it never got there.
            qDebug() << "LichessClient: move" << uciMove << "got no clear answer: HTTP" << httpStatus << reply->errorString();
            if (pending)
                resyncGame();
            return;
        }
        emit networkError(QStringLiteral("Move failed: ") + reply->errorString());
        // Lichess answered that it didn't play it: our board takes it back (if the stream hasn't
        // shown it meanwhile).
        if (pending)
            replaceMoveList(_lastMovesList, true);
    });
}

void LichessClient::postGameAction(const QString& path)
{
    auto* reply = _nam.post(makeRequest(path), QByteArray());
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void LichessClient::resign(const QString& gameId)
{
    postGameAction(QStringLiteral("/api/board/game/") + gameId + QStringLiteral("/resign"));
}

void LichessClient::offerDraw(const QString& gameId, bool accept)
{
    postGameAction(QStringLiteral("/api/board/game/") + gameId
                   + QStringLiteral("/draw/") + (accept ? QStringLiteral("yes") : QStringLiteral("no")));
}

void LichessClient::requestTakeback(const QString& gameId, bool accept)
{
    postGameAction(QStringLiteral("/api/board/game/") + gameId
                   + QStringLiteral("/takeback/") + (accept ? QStringLiteral("yes") : QStringLiteral("no")));
}

void LichessClient::abortGame(const QString& gameId)
{
    postGameAction(QStringLiteral("/api/board/game/") + gameId + QStringLiteral("/abort"));
}

void LichessClient::claimVictory(const QString& gameId)
{
    // On success the game stream sends gameState with status "timeout" → gameEnded.
    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/board/game/") + gameId + QStringLiteral("/claim-victory")),
        QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            emit networkError(QStringLiteral("Claim victory failed: ")
                              + apiErrorMessage(reply->readAll(), reply));
    });
}

void LichessClient::createRematch()
{
    if (!canRematch()) {
        emit networkError(QStringLiteral("There is no Lichess game to rematch"));
        return;
    }
    const QString color = _lastGame.playedAsWhite ? QStringLiteral("black") : QStringLiteral("white");
    if (_lastGame.opponentId.isEmpty())
        createOpenChallengeSec(_lastGame.clockLimitSec, _lastGame.clockIncrementSec, _lastGame.days,
                               color, _lastGame.variant);
    else
        challengeUser(_lastGame, color);
}

void LichessClient::challengePlayer(const QString& username, int limitSec, int incrementSec, int days, const QString& color)
{
    if (!isUsername(username)) { // it goes into the URL
        emit networkError(QStringLiteral("That isn't a Lichess username"));
        return;
    }
    RematchInfo game;
    game.opponentId        = username;
    game.opponentName      = username;
    game.clockLimitSec     = limitSec;
    game.clockIncrementSec = incrementSec;
    game.days              = days;
    challengeUser(game, color);
}

// static
bool LichessClient::isUsername(const QString& text)
{
    static const QRegularExpression username(QRegularExpression::anchoredPattern(QStringLiteral("[a-zA-Z0-9_-]{2,30}")));
    // /api/challenge/open and /api/challenge/ai are other calls.
    return username.match(text).hasMatch() && text != QLatin1String("open") && text != QLatin1String("ai");
}

void LichessClient::challengeUser(const RematchInfo& game, const QString& color)
{
    stopDirectChallenge();
    _directOpponentName = game.opponentName;

    QUrlQuery body;
    addTimeControl(body, game.clockLimitSec, game.clockIncrementSec, game.days);
    body.addQueryItem(QStringLiteral("rated"),   QStringLiteral("false"));
    body.addQueryItem(QStringLiteral("color"),   color);
    body.addQueryItem(QStringLiteral("variant"), game.variant);
    // With keepAliveStream the challenge lives while this request is open, and the server ends it
    // with {"done":"accepted"|...}.
    _directKeepAlive = needsKeepAlive(game);
    if (_directKeepAlive)
        body.addQueryItem(QStringLiteral("keepAliveStream"), QStringLiteral("true"));

    QNetworkRequest req = makeRequest(QStringLiteral("/api/challenge/") + game.opponentId);
    req.setRawHeader("Accept", "application/x-ndjson");
    _directChallengeAccepted = false;
    _directChallengeDone     = false;
    _directChallengeReply = _nam.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    auto* reply = _directChallengeReply;

    auto httpFailed = [reply]() {
        return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 400;
    };

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, httpFailed]() {
        if (reply != _directChallengeReply || httpFailed())
            return; // an error body is read once, in finished()
        while (reply->canReadLine())
            handleDirectChallengeLine(reply->readLine().trimmed());
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, httpFailed]() {
        reply->deleteLater();
        if (reply != _directChallengeReply)
            return; // replaced or stopped by us
        _directChallengeReply = nullptr;

        const QByteArray rest = reply->readAll().trimmed();
        qDebug() << "LichessClient: challenge stream finished | HTTP"
                 << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << "| error:" << reply->error() << "|" << rest.left(300);

        if (httpFailed()) {
            stopWaitingForDirectChallenge();
            emit networkError(QStringLiteral("Challenge failed: ") + apiErrorMessage(rest, reply));
            return;
        }
        if (reply->error() == QNetworkReply::NoError)
            for (const QByteArray& line : rest.split('\n'))
                handleDirectChallengeLine(line.trimmed());

        // Once accepted, the game exists: however the stream ended, waitForGameStart()
        // finds it. Without a final {"done":...} the challenge is gone (connection lost,
        // or closed early), so stop waiting instead of showing "Waiting…" forever.
        if (_directChallengeAccepted || _directChallengeDone)
            return;
        const bool wasSent = !_directChallengeId.isEmpty();
        stopWaitingForDirectChallenge();
        if (wasSent && reply->error() == QNetworkReply::NoError)
            emit challengeCanceled();
        else
            emit networkError(QStringLiteral("Challenge failed: ")
                              + (reply->error() == QNetworkReply::NoError
                                 ? QStringLiteral("no response from Lichess")
                                 : apiErrorMessage(rest, reply)));
    });
}

void LichessClient::handleDirectChallengeLine(const QByteArray& line)
{
    if (line.isEmpty())
        return; // keep-alive
    qDebug() << "LichessClient: challenge stream line:" << line.left(300);

    const auto doc = QJsonDocument::fromJson(line);
    if (!doc.isObject())
        return;
    const QJsonObject obj = doc.object();

    const QString done = obj.value(QStringLiteral("done")).toString();
    if (!done.isEmpty()) {
        _directChallengeDone = true;
        if (done == QLatin1String("accepted")) {
            _directChallengeAccepted = true;
            checkGameReady(); // waitForGameStart() is running; probe now instead of in 3 s
            return;
        }
        // Declined, or canceled on the server side (e.g. expired). Our own cancel goes
        // through cancelChallenge(), which stops this stream before the server answers.
        stopWaitingForDirectChallenge();
        if (done == QLatin1String("declined"))
            emit challengeDeclined(_directOpponentName);
        else
            emit challengeCanceled();
        return;
    }

    // First line: the created challenge (older API versions wrap it in "challenge").
    const QJsonObject challenge = obj.contains(QStringLiteral("challenge"))
        ? obj.value(QStringLiteral("challenge")).toObject() : obj;
    const QString id = challenge.value(QStringLiteral("id")).toString();
    if (id.isEmpty() || !_directChallengeId.isEmpty())
        return;

    // The game id is the challenge id. We are the challenger, so there is nothing to accept.
    _directChallengeId = id;
    if (!_directKeepAlive) {
        _directChallengeDone = true;
        emit directChallengeLeft(_directOpponentName);
        return;
    }
    _currentGameId     = id;
    emit directChallengeSent(id, _directOpponentName);
    waitForGameStart(id, false);
}

void LichessClient::stopWaitingForDirectChallenge()
{
    _pollTimer.stop();
    stopEventStream();
    _directChallengeId.clear();
    if (_watchingForRematch)
        openEventStream(); // the opponent may still offer a rematch themselves
}

void LichessClient::watchForRematch()
{
    if (!canRematch() || _lastGame.opponentId.isEmpty() || _lastGame.gameId.isEmpty())
        return; // an anonymous opponent can't offer one
    _watchingForRematch = true;
    _rematchOfferId.clear();
    if (!_eventStreamReply)
        openEventStream();
}

// Lichess shows the opponent's rematch offer as a challenge to us: its id is the next game's id,
// rematchOf the finished game. Not stored: it reaches only an event stream open at the time.
// static
bool LichessClient::isRematchOffer(const QJsonObject& challenge, const QString& lastGameId,
                                   const QString& username)
{
    const QString challenger = challenge.value(QStringLiteral("challenger")).toObject()
                                   .value(QStringLiteral("id")).toString();
    return !lastGameId.isEmpty()
        && challenge.value(QStringLiteral("rematchOf")).toString() == lastGameId
        && !challenge.value(QStringLiteral("id")).toString().isEmpty()
        && (username.isEmpty() || challenger != username); // the stream shows our own offers too
}

void LichessClient::handleRematchOffer(const QString& type, const QJsonObject& challenge)
{
    if (!_watchingForRematch)
        return;
    const QString id = challenge.value(QStringLiteral("id")).toString();
    if (type == QLatin1String("challengeCanceled")) {
        if (!id.isEmpty() && id == _rematchOfferId) {
            _rematchOfferId.clear();
            emit rematchOfferCanceled();
        }
        return;
    }
    if (!isRematchOffer(challenge, _lastGame.gameId, _username))
        return;
    qDebug() << "LichessClient: rematch offered" << id << "| rematch of" << _lastGame.gameId;
    _rematchOfferId = id;
    emit rematchOffered();
}

void LichessClient::acceptRematch()
{
    const QString id = _rematchOfferId;
    if (id.isEmpty())
        return;
    stopStream(); // also withdraws our own rematch challenge, if one waits
    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/challenge/") + id + QStringLiteral("/accept")),
        QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        qDebug() << "LichessClient: accept rematch" << id << "| HTTP"
                 << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << "| body:" << body.left(200);
        if (reply->error() == QNetworkReply::NoError)
            return;
        if (_waitForGameId == id) {
            _pollTimer.stop();
            stopEventStream();
        }
        emit networkError(QStringLiteral("Rematch failed: ") + apiErrorMessage(body, reply));
    });
    waitForGameStart(id, false); // the new game has the offer's id
}

void LichessClient::declineRematch()
{
    if (_rematchOfferId.isEmpty())
        return;
    postGameAction(QStringLiteral("/api/challenge/") + _rematchOfferId + QStringLiteral("/decline"));
    _rematchOfferId.clear();
}

void LichessClient::stopDirectChallenge()
{
    if (_directChallengeReply) {
        auto* reply = _directChallengeReply;
        _directChallengeReply = nullptr; // before abort(): its finished() handler must ignore it
        reply->abort();                  // closing the stream cancels the challenge on Lichess
    }
    _directChallengeId.clear();
}

// static
RematchInfo LichessClient::parseRematchInfo(const QJsonObject& gameFull, const QString& username)
{
    RematchInfo info;
    info.gameId = gameFull.value(QStringLiteral("id")).toString();
    const QJsonObject white = gameFull.value(QStringLiteral("white")).toObject();
    const QJsonObject black = gameFull.value(QStringLiteral("black")).toObject();
    info.playedAsWhite = resolveIsWhite(gameFull.value(QStringLiteral("myColor")).toString(),
                                        white.value(QStringLiteral("id")).toString(), username);

    const QJsonObject opponent = info.playedAsWhite ? black : white;
    if (opponent.contains(QStringLiteral("aiLevel")))
        return info; // Lichess AI, not a person: no rematch

    // An anonymous player has no "id".
    info.opponentId   = opponent.value(QStringLiteral("id")).toString();
    info.opponentName = opponent.value(QStringLiteral("name")).toString();
    if (info.opponentName.isEmpty())
        info.opponentName = info.opponentId.isEmpty() ? QStringLiteral("Anonymous") : info.opponentId;

    const QJsonObject clock = gameFull.value(QStringLiteral("clock")).toObject();
    if (!clock.isEmpty()) {
        info.clockLimitSec     = clock.value(QStringLiteral("initial")).toInt() / 1000;
        info.clockIncrementSec = clock.value(QStringLiteral("increment")).toInt() / 1000;
    }
    else {
        info.days = gameFull.value(QStringLiteral("daysPerTurn")).toInt();
    }

    const QString variant = gameFull.value(QStringLiteral("variant")).toObject()
                                .value(QStringLiteral("key")).toString();
    if (!variant.isEmpty())
        info.variant = variant;

    info.valid = true;
    return info;
}

void LichessClient::stopStream()
{
    _watchingForRematch = false;
    _rematchOfferId.clear();
    stopDirectChallenge();
    if (_openChallengeReply) {
        auto* reply = _openChallengeReply;
        _openChallengeReply = nullptr; // its finished() handler must ignore it
        reply->abort();
    }
    if (_acceptReply) {
        auto* reply = _acceptReply;
        _acceptReply = nullptr; // its finished() handler must not start the stream
        reply->abort();
    }
    _pollTimer.stop();
    stopProbe();
    stopEventStream();
    _gameStream->close();
    _postsInFlight.clear();
    _currentGameId.clear();
    _lastMovesList.clear();
    _sentMoves.clear();
}

QString LichessClient::gameIdFromUrl(const QString& urlOrId) const
{
    // Matches: https://lichess.org/XXXXXXXX, …/XXXXXXXX/white, and the challenge links Lichess
    // gives out: https://lichess.org/game/XXXXXXXX?color=black. Also accepts a bare 8-char ID.
    static const QRegularExpression re(
        QStringLiteral("(?:lichess\\.org/)(?:game/)?([A-Za-z0-9]{8})"));
    const auto match = re.match(urlOrId);
    if (match.hasMatch())
        return match.captured(1);
    // Bare ID (8 alphanumeric chars)
    static const QRegularExpression bareId(QStringLiteral("^[A-Za-z0-9]{8}$"));
    if (bareId.match(urlOrId.trimmed()).hasMatch())
        return urlOrId.trimmed();
    return {};
}

// static
QString LichessClient::colorFromUrl(const QString& url)
{
    static const QRegularExpression re(
        QStringLiteral("(?:[?&]color=|lichess\\.org/(?:game/)?[A-Za-z0-9]{8}/)(white|black)\\b"));
    const auto match = re.match(url);
    return match.hasMatch() ? match.captured(1) : QString();
}

// static
bool LichessClient::resolveIsWhite(const QString& myColor, const QString& whiteId, const QString& username)
{
    if (!myColor.isEmpty())
        return myColor == QLatin1String("white");
    if (!username.isEmpty())
        return whiteId == username; // Lichess IDs and stored username are both lowercase
    return true; // last resort: unknown color, assume white
}

// static
QString LichessClient::encryptToken(const QString& plaintext)
{
#ifdef WIN32
    const QByteArray utf8 = plaintext.toUtf8();
    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(utf8.data()));
    dataIn.cbData = static_cast<DWORD>(utf8.size());

    DATA_BLOB dataOut{};
    if (!CryptProtectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
        return {};

    const QByteArray encrypted(reinterpret_cast<const char*>(dataOut.pbData),
                                static_cast<int>(dataOut.cbData));
    LocalFree(dataOut.pbData);
    return QString::fromLatin1(encrypted.toBase64());
#else
    return QString::fromLatin1(plaintext.toUtf8().toBase64());
#endif
}

// static
QString LichessClient::decryptToken(const QString& base64Ciphertext)
{
    if (base64Ciphertext.isEmpty()) return {};
#ifdef WIN32
    const QByteArray cipher = QByteArray::fromBase64(base64Ciphertext.toLatin1());
    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(cipher.data()));
    dataIn.cbData = static_cast<DWORD>(cipher.size());

    DATA_BLOB dataOut{};
    if (!CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut))
        return {};

    const QString result = QString::fromUtf8(
        reinterpret_cast<const char*>(dataOut.pbData),
        static_cast<int>(dataOut.cbData));
    LocalFree(dataOut.pbData);
    return result;
#else
    return QString::fromUtf8(QByteArray::fromBase64(base64Ciphertext.toLatin1()));
#endif
}
