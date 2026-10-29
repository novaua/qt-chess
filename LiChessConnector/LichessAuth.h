#pragma once
#include "lichessconnector_global.h"
#include <QObject>
#include <QTcpServer>
#include <QNetworkAccessManager>

class QNetworkReply;

class LICHESSCONNECTOR_EXPORT LichessAuth : public QObject
{
	Q_OBJECT

public:
	explicit LichessAuth(QObject* parent = nullptr);
	void startLogin();                    // OAuth in the browser
	void useToken(const QString& token);  // a personal API token instead of OAuth
	void cancel();                        // stop waiting for the browser; no signal follows

signals:
	void loginSucceeded(const QString& token, const QString& username);
	void loginFailed(const QString& error);

private slots:
	void onNewConnection();

private:
	void exchangeCode(const QString& code);
	void fetchUsername(const QString& token);

	QTcpServer            _server;
	QNetworkAccessManager _nam;
	QNetworkReply*        _reply = nullptr; // token exchange or account fetch in flight
	QString _codeVerifier;
	QString _state;
	int     _port = 0;
};
