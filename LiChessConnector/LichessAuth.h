#pragma once
#include "lichessconnector_global.h"
#include "OnlineAuth.h"
#include <QObject>
#include <QTcpServer>
#include <QNetworkAccessManager>

class QNetworkReply;

// Lichess's sign-in (LI-18: its OnlineAuth): OAuth with PKCE in the browser (a local QTcpServer takes the
// callback), or a personal API token; either ends with the account's username from /api/account.
class LICHESSCONNECTOR_EXPORT LichessAuth : public OnlineAuth
{
	Q_OBJECT

public:
	explicit LichessAuth(QObject* parent = nullptr);
	void startLogin() override;                    // OAuth in the browser
	void useToken(const QString& token) override;  // a personal API token instead of OAuth
	void cancel() override;                        // stop waiting for the browser; no signal follows

private slots:
	void onNewConnection();

private:
	void exchangeCode(const QString& code);
	void fetchUsername(const QString& token);
	static QString errorMessage(const QNetworkReply* reply);

	QTcpServer            _server;
	QNetworkAccessManager _nam;
	QNetworkReply*        _reply = nullptr; // token exchange or account fetch in flight
	QString _codeVerifier;
	QString _state;
	int     _port = 0;
};
