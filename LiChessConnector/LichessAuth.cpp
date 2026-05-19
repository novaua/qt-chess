#include "stdafx.h"
#include "LichessAuth.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <windows.h>
#include <shellapi.h>

static const char* kClientId     = "qt-chess";
static const char* kAuthUrl      = "https://lichess.org/oauth";
static const char* kTokenUrl     = "https://lichess.org/api/token";
static const char* kAccountUrl   = "https://lichess.org/api/account";
static const char* kScope        = "board:play challenge:write";

static QString base64Url(const QByteArray& data)
{
	return data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

LichessAuth::LichessAuth(QObject* parent)
	: QObject(parent)
{
}

void LichessAuth::startLogin()
{
	// Generate code_verifier: 64 random bytes → Base64URL (86 chars, within RFC 7636 limits)
	QByteArray verifierBytes(64, Qt::Uninitialized);
	QRandomGenerator::global()->fillRange(
		reinterpret_cast<quint32*>(verifierBytes.data()),
		verifierBytes.size() / sizeof(quint32));
	_codeVerifier = base64Url(verifierBytes);

	// code_challenge = Base64URL(SHA-256(code_verifier))
	auto hash      = QCryptographicHash::hash(_codeVerifier.toUtf8(), QCryptographicHash::Sha256);
	auto challenge = base64Url(hash);

	// Random state for CSRF protection
	QByteArray stateBytes(8, Qt::Uninitialized);
	QRandomGenerator::global()->fillRange(reinterpret_cast<quint32*>(stateBytes.data()), 2);
	_state = base64Url(stateBytes);

	// Start local TCP server on an ephemeral port
	if (!_server.listen(QHostAddress::LocalHost, 0)) {
		emit loginFailed("Could not start local server: " + _server.errorString());
		return;
	}
	_port = _server.serverPort();
	connect(&_server, &QTcpServer::newConnection, this, &LichessAuth::onNewConnection);

	// Build authorization URL and open browser
	QUrl url(kAuthUrl);
	QUrlQuery q;
	q.addQueryItem("response_type",          "code");
	q.addQueryItem("client_id",              kClientId);
	q.addQueryItem("redirect_uri",           QString("http://localhost:%1/callback").arg(_port));
	q.addQueryItem("code_challenge_method",  "S256");
	q.addQueryItem("code_challenge",         challenge);
	q.addQueryItem("scope",                  kScope);
	q.addQueryItem("state",                  _state);
	url.setQuery(q);
	ShellExecuteW(nullptr, L"open",
		url.toString().toStdWString().c_str(),
		nullptr, nullptr, SW_SHOWNORMAL);
}

void LichessAuth::onNewConnection()
{
	QTcpSocket* socket = _server.nextPendingConnection();
	if (!socket) return;

	_server.close();

	connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
		// Parse "GET /callback?code=XXX&state=YYY HTTP/1.1" from the first line
		const QString firstLine = QString::fromUtf8(socket->readAll())
			.left(256).section('\n', 0, 0).trimmed();
		const int pathStart = firstLine.indexOf(' ') + 1;
		const int pathEnd   = firstLine.indexOf(' ', pathStart);
		const QUrl pathUrl("http://localhost" +
			firstLine.mid(pathStart, pathEnd - pathStart));
		QUrlQuery q(pathUrl.query());
		const QString code  = q.queryItemValue("code");
		const QString state = q.queryItemValue("state");

		// Write the success page before processing the code so the browser
		// sees it while the token exchange happens in the background
		const QByteArray body =
			"<!DOCTYPE html><html><body style='font-family:sans-serif;padding:2em'>"
			"<h2>&#10003; Authorized</h2>"
			"<p>You can close this tab and return to the app.</p>"
			"</body></html>";
		socket->write(
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"Content-Length: " + QByteArray::number(body.size()) + "\r\n"
			"Connection: close\r\n\r\n" + body);
		socket->flush();

		// Close gracefully once all bytes are sent
		connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
		socket->disconnectFromHost();

		if (code.isEmpty()) {
			emit loginFailed("No authorization code received");
			return;
		}
		if (state != _state) {
			emit loginFailed("State mismatch — possible CSRF attempt");
			return;
		}
		exchangeCode(code);
	});
}

void LichessAuth::exchangeCode(const QString& code)
{
	QNetworkRequest req{QUrl(kTokenUrl)};
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

	QUrlQuery body;
	body.addQueryItem("grant_type",    "authorization_code");
	body.addQueryItem("code",          code);
	body.addQueryItem("code_verifier", _codeVerifier);
	body.addQueryItem("redirect_uri",  QString("http://localhost:%1/callback").arg(_port));
	body.addQueryItem("client_id",     kClientId);

	QNetworkReply* reply = _nam.post(req, body.toString(QUrl::FullyEncoded).toUtf8());
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit loginFailed("Token exchange failed: " + reply->errorString());
			return;
		}
		const auto json = QJsonDocument::fromJson(reply->readAll()).object();
		const QString token = json.value("access_token").toString();
		if (token.isEmpty()) {
			emit loginFailed("No access_token in response");
			return;
		}
		fetchUsername(token);
	});
}

void LichessAuth::fetchUsername(const QString& token)
{
	QNetworkRequest req{QUrl(kAccountUrl)};
	req.setRawHeader("Authorization", ("Bearer " + token).toUtf8());

	QNetworkReply* reply = _nam.get(req);
	connect(reply, &QNetworkReply::finished, this, [this, reply, token]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit loginFailed("Account fetch failed: " + reply->errorString());
			return;
		}
		const auto json     = QJsonDocument::fromJson(reply->readAll()).object();
		const QString username = json.value("username").toString();
		if (username.isEmpty()) {
			emit loginFailed("Could not read username from account");
			return;
		}
		emit loginSucceeded(token, username);
	});
}
