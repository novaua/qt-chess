#include "stdafx.h"

#include "appservices.h"

#include "AppInfo.h"
#include "AvatarProvider.h"
#include "OnlineHub.h"
#include "UserManager.h"
#include "chessconnector.h"
#include "gamelibrary.h"
#include "layoutgate.h"

#include <QCoreApplication>
#include <QTimer>
#include <QtQml/qqml.h>

AppServices::AppServices(QObject* owner, bool offline, const QString& pluginDir)
	: _connector(new ChessConnector(owner))
	, _avatarProvider(new AvatarProvider(owner))
	, _userManager(new UserManager(owner))
	, _onlineHub(new OnlineHub(owner))
	, _appInfo(new AppInfo(owner))
	, _gameLibrary(new GameLibrary(owner))
{
	_connector->setAvatarProvider(_avatarProvider);
	_connector->setUserManager(_userManager);
	_connector->setGameLibrary(_gameLibrary);
	_gameLibrary->setUserManager(_userManager); // the active profile's games (and the old saves migrated)

	// LI-18: the online servers are plugins; without one, the app plays locally only.
	_onlineHub->loadPlugins(pluginDir);
	for (OnlineClient* client : _onlineHub->clients())
		client->setOffline(offline);
	_onlineClient = _onlineHub->current();
	// Until a profile has an account per server (LI-18 step 4), its one account is Lichess's: the profile's
	// sign-in and token go to Lichess's client only, never to another server's.
	_lichessAccountClient = _onlineHub->client(QStringLiteral("lichess"));
	if (_onlineClient)
		_connector->setOnlineClient(_onlineClient);
	if (_lichessAccountClient)
		_userManager->setOnlineServer(_lichessAccountClient->sharedServer()); // its sign-in
}

// static
QString AppServices::defaultPluginDir()
{
	return QCoreApplication::applicationDirPath() + QStringLiteral("/online");
}

void AppServices::start()
{
	// The engine looked for once the event loop runs (BL-25): the Analyse buttons are enabled only for
	// one that answered, from the first game page on (it used to wait for the Settings page).
	QTimer::singleShot(0, _appInfo, &AppInfo::probeEngine);

	QObject::connect(_userManager, &UserManager::activeUserChanged, &_guard, [this]() {
		_avatarProvider->setPlayerFromUser(_userManager->activeUserAvatar());
		updateLichessToken();
	});
	QObject::connect(_userManager, &UserManager::lichessChanged, &_guard, [this]() { updateLichessToken(); });
	updateLichessToken(); // load() sets the active user silently: seed the token now

	if (_lichessAccountClient) {
		QObject::connect(_lichessAccountClient, &OnlineClient::accountFetched, _userManager, &UserManager::setLichessAccount);
		QObject::connect(_lichessAccountClient, &OnlineClient::authExpired, _userManager, &UserManager::setLichessExpired);
	}
	// An online game left unfinished (Home, switch user) is now one of the ongoing games (LI-04).
	auto* connector = _connector;
	for (OnlineClient* client : _onlineHub->clients()) {
		QObject::connect(connector, &ChessConnector::isOnlineGameChanged, client, [connector, client]() {
			if (!connector->isOnlineGame())
				client->refreshOngoingGames();
		});
		QObject::connect(client, &OnlineClient::gameEnded, client, [client]() {
			QTimer::singleShot(3000, client, [client]() {
				client->refreshAccount();      // the new rating
				client->refreshOngoingGames(); // that game is no longer ongoing
			});
		});
	}
}

void AppServices::registerQmlTypes() const
{
	// QML: `import ChessPlusPlus`, then e.g. ChessConnector.startLocalGame(…), UserManager.login(…).
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "ChessConnector", _connector);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AvatarProvider", _avatarProvider);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "UserManager", _userManager);
	// LI-18: the online servers; Online.current is the client the app plays on (null without a plugin).
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "Online", _onlineHub);
	qmlRegisterUncreatableType<OnlineClient>("ChessPlusPlus", 1, 0, "OnlineClient", QStringLiteral("Online.current"));
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AppInfo", _appInfo);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "GameLibrary", _gameLibrary); // BL-19
	qmlRegisterType<LayoutGate>("ChessPlusPlus", 1, 0, "LayoutGate"); // BL-11
}

// The active profile's Lichess token goes to Lichess's client (none while it is expired; nowhere without
// the Lichess plugin). A new token: fetch the account for the ratings; a 401 on any call, or a token this
// Windows account can't decrypt (UserManager::lichessToken), marks the link expired.
void AppServices::updateLichessToken()
{
	auto* client = _lichessAccountClient;
	if (!client)
		return;
	const QString token = _userManager->lichessToken();
	client->setUsername(_userManager->lichessUsername());
	if (token == _lastToken)
		return;
	_lastToken = token;
	client->setToken(token);
	if (!token.isEmpty()) { // each does nothing offline (setOffline)
		client->refreshAccount();
		client->refreshOngoingGames(); // LI-04: Continue and the Online page
		client->startEventStream();    // BL-10: challenges, games started elsewhere
	}
}
