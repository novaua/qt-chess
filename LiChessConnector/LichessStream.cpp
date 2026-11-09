#include "stdafx.h"
#include "LichessStream.h"
#include <QNetworkAccessManager>
#include <algorithm>

LichessStream::LichessStream(QNetworkAccessManager& nam, const QString& name, QObject* parent)
    : QObject(parent), _nam(nam), _name(name)
{
    _retry.setSingleShot(true);
    connect(&_retry, &QTimer::timeout, this, &LichessStream::start);
    _watchdog.setSingleShot(true);
    _watchdog.setInterval(20000);
    connect(&_watchdog, &QTimer::timeout, this, [this]() {
        qDebug() << "LichessClient: nothing from the" << _name << "for" << _watchdog.interval() << "ms";
        drop();
        lose();
    });
}

void LichessStream::setTiming(int silenceMs, int firstRetryMs)
{
    _watchdog.setInterval(silenceMs);
    _firstRetryMs = firstRetryMs;
}

void LichessStream::open(const QNetworkRequest& request)
{
    close();
    _request = request;
    start();
}

void LichessStream::reopen()
{
    if (_lost || _request.url().isEmpty())
        return; // a reconnect is on its way anyway, or nothing to open
    qDebug() << "LichessClient:" << _name << "opened again";
    drop();
    start();
}

void LichessStream::close()
{
    _retry.stop();
    drop();
    _request = QNetworkRequest();
    _reconnect = false;
    _lost = false;
    _attempt = 0;
}

void LichessStream::setReconnect(bool reconnect)
{
    _reconnect = reconnect;
    if (reconnect && _reply)
        _watchdog.start();
    if (!reconnect) {
        _watchdog.stop();
        _retry.stop();
    }
}

void LichessStream::connected()
{
    _attempt = 0;
    if (_lost) {
        _lost = false;
        emit connectionChanged(true);
    }
}

void LichessStream::start()
{
    _reply = _nam.get(_request);
    auto* reply = _reply;
    if (_reconnect)
        _watchdog.start();

    // Every line, also the keep-alive, shows the connection is alive. A receiver may close the stream
    // (the game has ended): then the rest isn't read.
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        while (reply == _reply && reply->canReadLine()) {
            const QByteArray text = reply->readLine().trimmed();
            qDebug() << "LichessClient:" << _name << "line:" << text.left(300);
            emit line(text);
        }
        if (reply == _reply && _reconnect)
            _watchdog.start();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != _reply)
            return; // closed, or replaced by reopen()
        // A 401 may come before LichessClient's own check has marked the reply.
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool authExpired = reply->property("authExpiredReported").toBool() || status == 401;
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        _watchdog.stop();
        _reply = nullptr;
        reply->deleteLater();
        if (!_reconnect) {
            emit ended(error, errorString, authExpired);
            return;
        }
        if (authExpired || (400 <= status && status < 500)) {
            qDebug() << "LichessClient:" << _name << "refused: HTTP" << status;
            close(); // reopen() won't ask again either; open() a new one to try later
            emit stopped(authExpired, status);
            return;
        }
        lose();
    });

    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError code) {
        qDebug() << "LichessClient:" << _name << "error | HTTP"
                 << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << "| code:" << code << "|" << reply->errorString();
    });
}

void LichessStream::drop()
{
    _watchdog.stop();
    if (!_reply)
        return;
    auto* reply = _reply;
    _reply = nullptr; // before abort(): its handlers must ignore it
    reply->abort();
    reply->deleteLater();
}

void LichessStream::lose()
{
    const int delay = (std::min)(_firstRetryMs << (std::min)(_attempt, 4), 15000); // windows.h has a min macro
    ++_attempt;
    qDebug() << "LichessClient:" << _name << "lost; reconnecting in" << delay << "ms";
    if (!_lost) {
        _lost = true;
        emit connectionChanged(false);
    }
    _retry.start(delay);
}
