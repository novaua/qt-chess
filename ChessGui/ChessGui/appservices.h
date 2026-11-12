#pragma once

#include <QObject>
#include <QString>

class AppInfo;
class AvatarProvider;
class ChessConnector;
class LichessClient;
class UserManager;

// The app's services and how they work together: made once per app (main.cpp) or per test file
// (ChessGuiTests' TestSetup, BL-05), so the UI tests run the app's own wiring, not a copy of it.
// The services are children of `owner` and outlive the QML engine that shows them.
class AppServices
{
public:
	// `offline`: Lichess is never called (screenshot scenes, UI tests); its token is a placeholder there.
	AppServices(QObject* owner, bool offline);

	AppServices(const AppServices&) = delete;
	AppServices& operator=(const AppServices&) = delete;

	ChessConnector* connector() const { return _connector; }
	AvatarProvider* avatarProvider() const { return _avatarProvider; }
	UserManager* userManager() const { return _userManager; }
	LichessClient* lichessClient() const { return _lichessClient; }
	AppInfo* appInfo() const { return _appInfo; }

	// The active profile's Lichess token to the client, the account, the ongoing games, the event
	// stream. After the profiles are loaded (and a screenshot scene's are seeded): it reads the active one.
	void start();

	// `import ChessPlusPlus`: the five services as singletons, and LayoutGate. Once per process.
	void registerQmlTypes() const;

private:
	void updateLichessToken();

	ChessConnector* _connector;
	AvatarProvider* _avatarProvider;
	UserManager* _userManager;
	LichessClient* _lichessClient;
	AppInfo* _appInfo;
	QString _lastToken;
	// The context of the connections that call back into this object: they go when it does (last
	// member, so first destroyed), while the services, children of the app, live on a little longer.
	QObject _guard;
};
