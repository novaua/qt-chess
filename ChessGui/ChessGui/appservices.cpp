#include "stdafx.h"

#include "appservices.h"

#include "AppInfo.h"
#include "AvatarProvider.h"
#include "LichessClient.h"
#include "UserManager.h"
#include "chessconnector.h"
#include "layoutgate.h"

#include <QTimer>
#include <QtQml/qqml.h>

AppServices::AppServices(QObject* owner, bool offline)
	: _connector(new ChessConnector(owner))
	, _avatarProvider(new AvatarProvider(owner))
	, _userManager(new UserManager(owner))
	, _lichessClient(new LichessClient(owner))
	, _appInfo(new AppInfo(owner))
{
	_connector->setAvatarProvider(_avatarProvider);
	_connector->setUserManager(_userManager);
	_connector->setLichessClient(_lichessClient);
	_lichessClient->setOffline(offline);
}

void AppServices::start()
{
	QObject::connect(_userManager, &UserManager::activeUserChanged, &_guard, [this]() {
		_avatarProvider->setPlayerFromUser(_userManager->activeUserAvatar());
		updateLichessToken();
	});
	QObject::connect(_userManager, &UserManager::lichessChanged, &_guard, [this]() { updateLichessToken(); });
	updateLichessToken(); // load() sets the active user silently: seed the token now

	QObject::connect(_lichessClient, &LichessClient::accountFetched, _userManager, &UserManager::setLichessAccount);
	QObject::connect(_lichessClient, &LichessClient::authExpired, _userManager, &UserManager::setLichessExpired);
	// A Lichess game left unfinished (Home, switch user) is now one of the ongoing games (LI-04).
	auto* connector = _connector;
	auto* lichess = _lichessClient;
	QObject::connect(connector, &ChessConnector::isOnlineGameChanged, lichess, [connector, lichess]() {
		if (!connector->isOnlineGame())
			lichess->refreshOngoingGames();
	});
	QObject::connect(lichess, &LichessClient::gameEnded, lichess, [lichess]() {
		QTimer::singleShot(3000, lichess, [lichess]() {
			lichess->refreshAccount();      // the new rating
			lichess->refreshOngoingGames(); // that game is no longer ongoing
		});
	});
}

void AppServices::registerQmlTypes() const
{
	// QML: `import ChessPlusPlus`, then e.g. ChessConnector.startLocalGame(…), UserManager.login(…).
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "ChessConnector", _connector);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AvatarProvider", _avatarProvider);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "UserManager", _userManager);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "LichessClient", _lichessClient);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AppInfo", _appInfo);
	qmlRegisterType<LayoutGate>("ChessPlusPlus", 1, 0, "LayoutGate"); // BL-11
}

// The active profile's Lichess token goes to the client (none while it is expired). A new token:
// fetch the account for the ratings; a 401 on any call marks the link expired.
void AppServices::updateLichessToken()
{
	const QString token = _userManager->lichessConnected()
		? LichessClient::decryptToken(_userManager->lichessTokenEncrypted()) : QString();
	_lichessClient->setUsername(_userManager->lichessUsername());
	if (token == _lastToken)
		return;
	_lastToken = token;
	_lichessClient->setToken(token);
	if (!token.isEmpty()) { // each does nothing offline (setOffline)
		_lichessClient->refreshAccount();
		_lichessClient->refreshOngoingGames(); // LI-04: Continue and the Online page
		_lichessClient->startEventStream();    // BL-10: challenges, games started elsewhere
	}
}
