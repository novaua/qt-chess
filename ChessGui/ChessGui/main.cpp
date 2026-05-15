#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "chessconnector.h"
#include "AvatarProvider.h"
#include "UserManager.h"
#include "LichessClient.h"

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/app/pics/chess_icon.svg"));

	QQmlApplicationEngine engine;

	auto* connector      = new ChessConnector(&engine);
	auto* avatarProvider = new AvatarProvider(&engine);
	auto* userManager    = new UserManager(&engine);
	auto* lichessClient  = new LichessClient(&engine);

	connector->setAvatarProvider(avatarProvider);
	connector->setUserManager(userManager);
	connector->setLichessClient(lichessClient);

	QObject::connect(connector, &ChessConnector::newGameStarted,
		avatarProvider, &AvatarProvider::randomize);

	QObject::connect(userManager, &UserManager::activeUserChanged, [=]() {
		avatarProvider->setPlayerFromUser(userManager->activeUserAvatar());
		lichessClient->setToken(
		    LichessClient::decryptToken(userManager->lichessTokenEncrypted()));
	});

	engine.rootContext()->setContextProperty("chessConnector", connector);
	engine.rootContext()->setContextProperty("avatarProvider",  avatarProvider);
	engine.rootContext()->setContextProperty("userManager",     userManager);
	engine.rootContext()->setContextProperty("lichessClient",   lichessClient);

	engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));

	return app.exec();
}
