#pragma once
#include "lichessconnector_global.h"
#include <QObject>
#include <QTcpServer>
#include <QNetworkAccessManager>

class LICHESSCONNECTOR_EXPORT LichessAuth : public QObject
{
	Q_OBJECT

public:
	explicit LichessAuth(QObject* parent = nullptr);
	void startLogin();

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
	QString _codeVerifier;
	QString _state;
	int     _port = 0;
};
