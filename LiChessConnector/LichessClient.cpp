#include "stdafx.h"
#include "LichessClient.h"

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
            [](QNetworkReply* reply, QAuthenticator*) { reply->abort(); });
}

void LichessClient::setToken(const QString& decryptedToken)
{
    _token = decryptedToken;
    qDebug() << "LichessClient: token" << (_token.isEmpty() ? "cleared" : QString("set(%1 chars)").arg(_token.size()));
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

void LichessClient::createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant)
{
    QUrlQuery body;
    if (minutes > 0) {
        body.addQueryItem(QStringLiteral("clock.limit"),     QString::number(minutes * 60));
        body.addQueryItem(QStringLiteral("clock.increment"), QString::number(increment));
    }
    body.addQueryItem(QStringLiteral("color"),   color);
    body.addQueryItem(QStringLiteral("variant"), variant);

    auto* reply = _nam.post(makeRequest(QStringLiteral("/api/challenge/open")),
                            body.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, color]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "LichessClient: createOpenChallenge HTTP" << httpStatus << raw.left(500);

        if (reply->error() != QNetworkReply::NoError) {
            emit networkError(reply->errorString());
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

void LichessClient::waitForGameStart(const QString& gameId)
{
    stopEventStream();
    _pollTimer.stop();
    _waitForGameId = gameId;

    // Must call accept to register the creator as a Board API player; without it
    // the board-game stream endpoint returns 404 even after the opponent joins.
    auto* reg = _nam.post(
        makeRequest(QStringLiteral("/api/challenge/") + gameId
                    + QStringLiteral("/accept")),
        QByteArray());
    connect(reg, &QNetworkReply::finished, this, [reg]() { reg->deleteLater(); });

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

void LichessClient::stopStream()
{
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
