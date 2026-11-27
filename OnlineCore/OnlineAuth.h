#pragma once
#include "onlinecore_global.h"
#include <QObject>

// LI-18: signing in to an online server, as that server does it: in the browser (Lichess's OAuth), or with a
// token the player pastes. Made by the server (OnlineServer::createAuth), so it asks only that server's
// addresses: a token is checked where it will be used. Ends with one signal, unless canceled.
class ONLINECORE_EXPORT OnlineAuth : public QObject
{
	Q_OBJECT

public:
	using QObject::QObject;

	virtual void startLogin() = 0;                    // in the browser
	virtual void useToken(const QString& token) = 0;  // a personal API token instead
	virtual void cancel() = 0;                        // stop waiting; no signal follows

signals:
	void loginSucceeded(const QString& token, const QString& username);
	void loginFailed(const QString& error);
};
