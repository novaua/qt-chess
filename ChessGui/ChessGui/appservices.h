#pragma once

#include <QObject>
#include <QString>

class AppInfo;
class AvatarProvider;
class ChessConnector;
class GameLibrary;
class OnlineClient;
class OnlineHub;
class UserManager;

// The app's services and how they work together: made once per app (main.cpp) or per test file
// (ChessGuiTests' TestSetup, BL-05), so the UI tests run the app's own wiring, not a copy of it.
// The services are children of `owner` and outlive the QML engine that shows them.
class AppServices
{
public:
	// `offline`: no online server is called (screenshot scenes, UI tests); the token is a placeholder there.
	// `pluginDir`: the online servers' plugins (LI-18): defaultPluginDir(), or the tests' (none: no server).
	AppServices(QObject* owner, bool offline, const QString& pluginDir);
	static QString defaultPluginDir(); // the folder "online" beside the executable

	AppServices(const AppServices&) = delete;
	AppServices& operator=(const AppServices&) = delete;

	ChessConnector* connector() const { return _connector; }
	AvatarProvider* avatarProvider() const { return _avatarProvider; }
	UserManager* userManager() const { return _userManager; }
	OnlineHub* onlineHub() const { return _onlineHub; }
	// The client the app plays on (OnlineHub::current), null without a plugin.
	OnlineClient* onlineClient() const { return _onlineClient; }
	// Lichess's client: the profile's Lichess account (its token) goes there only; null without the plugin.
	OnlineClient* lichessAccountClient() const { return _lichessAccountClient; }
	AppInfo* appInfo() const { return _appInfo; }
	GameLibrary* gameLibrary() const { return _gameLibrary; }

	// The active profile's Lichess token to Lichess's client, the account, the ongoing games, the event
	// stream. After the profiles are loaded (and a screenshot scene's are seeded): it reads the active one.
	void start();

	// `import ChessPlusPlus`: the six services as singletons (Online: the OnlineHub), and LayoutGate. Once per process.
	void registerQmlTypes() const;

private:
	void updateLichessToken();

	ChessConnector* _connector;
	AvatarProvider* _avatarProvider;
	UserManager* _userManager;
	OnlineHub* _onlineHub; // LI-18: the plugins and their clients
	AppInfo* _appInfo;
	GameLibrary* _gameLibrary; // BL-19: the active profile's kept games
	OnlineClient* _onlineClient = nullptr;
	OnlineClient* _lichessAccountClient = nullptr;
	QString _lastToken;
	// The context of the connections that call back into this object: they go when it does (last
	// member, so first destroyed), while the services, children of the app, live on a little longer.
	QObject _guard;
};
