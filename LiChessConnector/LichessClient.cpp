#include "stdafx.h"
#include "LichessClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QRegularExpression>

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "Crypt32.lib")
#endif

static const QString kBaseUrl = QStringLiteral("https://lichess.org");

LichessClient::LichessClient(QObject* parent)
    : QObject(parent)
{
    // Prevent Qt from intercepting 401 responses and overriding our Bearer token.
    connect(&_nam, &QNetworkAccessManager::authenticationRequired,
            this, [this](QNetworkReply* reply, QAuthenticator*) { checkAuthExpired(reply, true); reply->abort(); });
    connect(&_nam, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply* reply) { checkAuthExpired(reply, false); });
}

// Any call made with our token that Lichess answers with 401: the token was revoked or expired.
// validateToken() sends a different token, so it doesn't count.
void LichessClient::checkAuthExpired(QNetworkReply* reply, bool unauthorized)
{
    if (_token.isEmpty() || reply->request().rawHeader("Authorization") != ("Bearer " + _token).toUtf8())
        return;
    if (unauthorized || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
        qDebug() << "LichessClient: 401 for" << reply->url().path() << "- the token is no longer valid";
        emit authExpired();
    }
}

void LichessClient::refreshAccount()
{
    if (_token.isEmpty()) return;
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
    _token = decryptedToken;
    qDebug() << "LichessClient: token" << (_token.isEmpty() ? "cleared" : QString("set(%1 chars)").arg(_token.size()));
    emit canRematchChanged();
}

void LichessClient::setUsername(const QString& username)
{
    _username = username.toLower(); // Lichess IDs are always lowercase
}

QNetworkRequest LichessClient::makeRequest(const QString& path) const
{
    qDebug() << "LichessClient: >>" << path
             << "| token:" << (_token.isEmpty() ? "MISSING" : QString("set(%1 chars)").arg(_token.size()));
    QNetworkRequest req(QUrl(kBaseUrl + path));
    if (!_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + _token).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    return req;
}

void LichessClient::validateToken(const QString& token)
{
    QNetworkRequest req(QUrl(kBaseUrl + "/api/account"));
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

void LichessClient::createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant)
{
    createOpenChallengeSec(minutes * 60, minutes > 0 ? increment : 0, 0, color, variant);
}

void LichessClient::createOpenChallengeSec(int limitSec, int incrementSec, int days,
                                           const QString& color, const QString& variant)
{
    QUrlQuery body;
    addTimeControl(body, limitSec, incrementSec, days);
    body.addQueryItem(QStringLiteral("color"),   color);
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
        waitForGameStart(gameId);
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
        emit challengeCanceled();
    });
}

void LichessClient::acceptChallenge(const QString& challengeId)
{
    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/challenge/") + challengeId + QStringLiteral("/accept")),
        QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, challengeId]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        qDebug() << "LichessClient: accept" << challengeId
                 << "| HTTP" << status << "| error:" << reply->error()
                 << "| body:" << body.left(200);
        reply->deleteLater();
        streamGame(challengeId);
    });
}

void LichessClient::waitForGameStart(const QString& gameId, bool registerAsPlayer)
{
    stopEventStream();
    _pollTimer.stop();
    _waitForGameId = gameId;

    // Must call accept to register the creator as a Board API player; without it
    // the board-game stream endpoint returns 404 even after the opponent joins.
    if (registerAsPlayer) {
        auto* reg = _nam.post(
            makeRequest(QStringLiteral("/api/challenge/") + gameId
                        + QStringLiteral("/accept")),
            QByteArray());
        connect(reg, &QNetworkReply::finished, this, [reg]() { reg->deleteLater(); });
    }

    QNetworkRequest req = makeRequest(QStringLiteral("/api/stream/event"));
    req.setRawHeader("Accept", "application/x-ndjson");
    _eventStreamReply = _nam.get(req);
    connect(_eventStreamReply, &QNetworkReply::readyRead, this, [this]() {
        handleEventStreamData(_eventStreamReply);
    });
    // Capture the reply by value to avoid dereferencing _eventStreamReply after
    // stopEventStream() nulls it (errorOccurred may fire asynchronously).
    auto* evReply = _eventStreamReply;
    connect(evReply, &QNetworkReply::errorOccurred, this,
            [evReply](QNetworkReply::NetworkError code) {
                const int status = evReply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << "LichessClient: event stream error | HTTP" << status
                         << "| code:" << code << "|" << evReply->errorString();
            });

    connect(&_pollTimer, &QTimer::timeout, this, &LichessClient::checkGameReady,
            Qt::UniqueConnection);
    _pollTimer.start(3000);
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
            if (!_waitForGameId.isEmpty() && gameId != _waitForGameId)
                continue;
            onGameStartDetected(gameId);
            return;
        }
    }
}

void LichessClient::checkGameReady()
{
    if (_waitForGameId.isEmpty()) {
        _pollTimer.stop();
        return;
    }

    // Abort any still-in-flight probe before launching a new one.
    if (_probeReply) {
        _probeReply->abort();
        _probeReply->deleteLater();
        _probeReply = nullptr;
    }

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
    _lastOppDraw     = false;
    _lastOppTakeback = false;

    QNetworkRequest req = makeRequest(
        QStringLiteral("/api/board/game/stream/") + gameId);
    req.setRawHeader("Accept", "application/x-ndjson");

    _streamReply = _nam.get(req);

    connect(_streamReply, &QNetworkReply::readyRead, this, [this]() {
        handleStreamData(_streamReply);
    });

    auto* sr = _streamReply;
    connect(sr, &QNetworkReply::errorOccurred, this,
            [this, sr](QNetworkReply::NetworkError code) {
                const int status = sr->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << "LichessClient: stream error | HTTP" << status << "| code:" << code
                         << "|" << sr->errorString();
                // OperationCanceledError is expected when the server closes the stream
                // normally (game over) or when we call stopStream() ourselves — not an error.
                if (code != QNetworkReply::OperationCanceledError)
                    emit networkError(sr->errorString());
            });
}

void LichessClient::handleStreamData(QNetworkReply* reply)
{
    while (reply->canReadLine()) {
        const QByteArray line = reply->readLine().trimmed();
        qDebug() << "LichessClient: stream line:" << line.left(300);
        if (line.isEmpty()) continue;

        const auto doc = QJsonDocument::fromJson(line);
        if (doc.isNull() || !doc.isObject()) continue;

        const QJsonObject obj  = doc.object();
        const QString     type = obj.value(QStringLiteral("type")).toString();

        if (type == QLatin1String("gameFull")) {
            const QJsonObject white = obj.value(QStringLiteral("white")).toObject();
            const QJsonObject black = obj.value(QStringLiteral("black")).toObject();
            const QJsonObject state = obj.value(QStringLiteral("state")).toObject();

            const QString myColor  = obj.value(QStringLiteral("myColor")).toString();
            const QString whiteId  = white.value(QStringLiteral("id")).toString();
            const bool    isWhite  = resolveIsWhite(myColor, whiteId, _username);
            _playingAsWhite = isWhite;
            qDebug() << "LichessClient: playing as" << (isWhite ? "white" : "black")
                     << "| myColor=" << myColor << "| whiteId=" << whiteId << "| username=" << _username;

            _lastGame = parseRematchInfo(obj, _username);
            emit canRematchChanged();

            const QJsonObject opponent = isWhite ? black : white;
            const QString opponentName      = opponent.value(QStringLiteral("name")).toString();
            const QString opponentAvatarUrl = {}; // Lichess doesn't provide avatar in stream

                    // Emit gameStarted first so the board is initialized before any moves are applied.
            _lastMovesList = state.value(QStringLiteral("moves")).toString()
                                 .split(QLatin1Char(' '), Qt::SkipEmptyParts);
            emit gameStarted(_currentGameId, isWhite, opponentName, opponentAvatarUrl);

            // Resync our board state with moves that arrived before we joined.
            if (!_lastMovesList.isEmpty())
                emit opponentMoveReceived(_lastMovesList.join(QLatin1Char(' ')));

            // Streaming a game that has already ended: Lichess sends only gameFull, then closes.
            const QString status = state.value(QStringLiteral("status")).toString();
            if (!status.isEmpty() && status != QLatin1String("started")
                    && status != QLatin1String("created")) {
                emit gameEnded(status, state.value(QStringLiteral("winner")).toString());
                return;
            }

        } else if (type == QLatin1String("opponentGone")) {
            const bool gone = obj.value(QStringLiteral("gone")).toBool();
            const int  claimIn = obj.contains(QStringLiteral("claimWinInSeconds"))
                ? obj.value(QStringLiteral("claimWinInSeconds")).toInt() : -1;
            emit opponentGone(gone, claimIn);

        } else if (type == QLatin1String("gameState")) {
            const QString moves  = obj.value(QStringLiteral("moves")).toString();
            const QString status = obj.value(QStringLiteral("status")).toString();

            if (status != QLatin1String("started") && !status.isEmpty()) {
                const QString winner = obj.value(QStringLiteral("winner")).toString();
                emit gameEnded(status, winner);
                return;
            }

            const QStringList allMoves = moves.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (int i = _lastMovesList.size(); i < allMoves.size(); ++i)
                emit opponentMoveReceived(allMoves[i]);
            _lastMovesList = allMoves;

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
    }
}

void LichessClient::postMove(const QString& gameId, const QString& uciMove)
{
    // Pre-track our own move so the stream echo doesn't re-apply it.
    _lastMovesList.append(uciMove);

    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/board/game/") + gameId
                    + QStringLiteral("/move/") + uciMove),
        QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply, uciMove]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            emit networkError(QStringLiteral("Move failed: ") + reply->errorString());
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

void LichessClient::challengeUser(const RematchInfo& game, const QString& color)
{
    stopDirectChallenge();

    QUrlQuery body;
    addTimeControl(body, game.clockLimitSec, game.clockIncrementSec, game.days);
    body.addQueryItem(QStringLiteral("rated"),   QStringLiteral("false"));
    body.addQueryItem(QStringLiteral("color"),   color);
    body.addQueryItem(QStringLiteral("variant"), game.variant);
    // Without keepAliveStream a direct challenge expires after 20 s. With it, the challenge
    // lives while this request is open, and the server ends it with {"done":"accepted"|...}.
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
            emit networkError(QStringLiteral("Rematch challenge failed: ") + apiErrorMessage(rest, reply));
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
            emit networkError(QStringLiteral("Rematch challenge failed: ")
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
            emit challengeDeclined(_lastGame.opponentName);
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
    _currentGameId     = id;
    emit directChallengeSent(id, _lastGame.opponentName);
    waitForGameStart(id, false);
}

void LichessClient::stopWaitingForDirectChallenge()
{
    _pollTimer.stop();
    stopEventStream();
    _directChallengeId.clear();
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
    stopDirectChallenge();
    if (_openChallengeReply) {
        auto* reply = _openChallengeReply;
        _openChallengeReply = nullptr; // its finished() handler must ignore it
        reply->abort();
    }
    _pollTimer.stop();
    if (_probeReply) {
        _probeReply->abort();
        _probeReply->deleteLater();
        _probeReply = nullptr;
    }
    stopEventStream();
    if (_streamReply) {
        _streamReply->abort();
        _streamReply->deleteLater();
        _streamReply = nullptr;
    }
    _currentGameId.clear();
    _lastMovesList.clear();
}

QString LichessClient::gameIdFromUrl(const QString& urlOrId) const
{
    // Matches: https://lichess.org/XXXXXXXX  or  https://lichess.org/XXXXXXXX/white
    // Also accepts a bare 8-char ID.
    static const QRegularExpression re(
        QStringLiteral("(?:lichess\\.org/)([A-Za-z0-9]{8})"));
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
