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
    qDebug() << "LichessClient: token" << (_token.isEmpty() ? "cleared" : "set, length=" + QString::number(_token.size()));
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

void LichessClient::createOpenChallenge(int minutes, int increment)
{
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("clock.limit"),     QString::number(minutes * 60));
    body.addQueryItem(QStringLiteral("clock.increment"), QString::number(increment));

    auto* reply = _nam.post(makeRequest(QStringLiteral("/api/challenge/open")),
                            body.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit networkError(reply->errorString());
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull()) { emit networkError(QStringLiteral("Invalid response")); return; }

        const QJsonObject challenge = doc.object().value(QStringLiteral("challenge")).toObject();
        const QString gameId  = challenge.value(QStringLiteral("id")).toString();
        // urlWhite is for white player, urlBlack for black — we give the joiner whichever they click
        const QString joinUrl = challenge.value(QStringLiteral("url")).toString();

        if (gameId.isEmpty()) { emit networkError(QStringLiteral("No game ID in response")); return; }

        _currentGameId = gameId;
        emit challengeCreated(gameId, joinUrl);
        streamGame(gameId);
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

void LichessClient::streamGame(const QString& gameId)
{
    stopStream();
    _currentGameId  = gameId;
    _lastMovesList  = {};

    QNetworkRequest req = makeRequest(
        QStringLiteral("/api/board/game/stream/") + gameId);
    req.setRawHeader("Accept", "application/x-ndjson");

    _streamReply = _nam.get(req);

    connect(_streamReply, &QNetworkReply::readyRead, this, [this]() {
        const int status = _streamReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "LichessClient: stream readyRead | HTTP" << status;
        handleStreamData(_streamReply);
    });

    connect(_streamReply, &QNetworkReply::errorOccurred, this,
            [this](QNetworkReply::NetworkError code) {
                const int status = _streamReply
                    ? _streamReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 0;
                qDebug() << "LichessClient: stream error | HTTP" << status
                         << "| code:" << code
                         << "|" << (_streamReply ? _streamReply->errorString() : "no reply");
                if (_streamReply)
                    emit networkError(_streamReply->errorString());
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

            const QString myColor = obj.value(QStringLiteral("myColor")).toString();
            const bool isWhite = (myColor == QLatin1String("white")) || myColor.isEmpty();

            const QJsonObject opponent = isWhite ? black : white;
            const QString opponentName      = opponent.value(QStringLiteral("name")).toString();
            const QString opponentAvatarUrl = {}; // Lichess doesn't provide avatar in stream

            // Emit gameStarted first so the board is initialized before any moves are applied.
            _lastMovesList = state.value(QStringLiteral("moves")).toString().trimmed();
            emit gameStarted(_currentGameId, isWhite, opponentName, opponentAvatarUrl);

            // Replay moves already on the board when joining mid-game (rejoin / late connect).
            if (!_lastMovesList.isEmpty())
                emit opponentMoveReceived(_lastMovesList);

        } else if (type == QLatin1String("gameState")) {
            const QString moves  = obj.value(QStringLiteral("moves")).toString();
            const QString status = obj.value(QStringLiteral("status")).toString();

            if (status != QLatin1String("started") && !status.isEmpty()) {
                const QString winner = obj.value(QStringLiteral("winner")).toString();
                emit gameEnded(status, winner);
                return;
            }

            // Find new moves since last update
            if (moves.length() > _lastMovesList.length()) {
                const QString newPart = moves.mid(_lastMovesList.isEmpty() ? 0
                                                                           : _lastMovesList.length() + 1);
                _lastMovesList = moves;
                // newPart may contain multiple space-separated moves if we were behind
                for (const QString& mv : newPart.split(QLatin1Char(' '), Qt::SkipEmptyParts))
                    emit opponentMoveReceived(mv);
            }
        }
    }
}

void LichessClient::postMove(const QString& gameId, const QString& uciMove)
{
    // Pre-track our own move so the stream echo doesn't re-apply it.
    _lastMovesList = _lastMovesList.isEmpty()
        ? uciMove
        : _lastMovesList + QLatin1Char(' ') + uciMove;

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

void LichessClient::resign(const QString& gameId)
{
    auto* reply = _nam.post(
        makeRequest(QStringLiteral("/api/board/game/") + gameId
                    + QStringLiteral("/resign")),
        QByteArray());
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void LichessClient::stopStream()
{
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
