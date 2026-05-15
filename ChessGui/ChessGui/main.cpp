#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "chessconnector.h"
#include "AvatarProvider.h"
#include "UserManager.h"
#include "LichessClient.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <windows.h>

static QFile* g_logFile = nullptr;

void customLogMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
	static const char* typeStr[] = { "DEBUG", "WARN ", "CRIT ", "FATAL", "INFO " };
	if (g_logFile) {
		QTextStream(g_logFile)
		    << '[' << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
		    << "] [" << typeStr[type] << "] " << msg << '\n';
	}
	OutputDebugStringW(reinterpret_cast<const WCHAR*>((msg + u'\n').utf16()));
}

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");

	const QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
	QDir().mkpath(logDir);
	g_logFile = new QFile(logDir + "/ChessPlusPlus_AppDebug.log");
	if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		delete g_logFile;
		g_logFile = nullptr;
	}
	qInstallMessageHandler(customLogMessageHandler);

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/app/pics/chess_icon.svg"));

	QQmlApplicationEngine engine;

	auto* connector = new ChessConnector(&engine);
	auto* avatarProvider = new AvatarProvider(&engine);
	auto* userManager = new UserManager(&engine);
	auto* lichessClient = new LichessClient(&engine);

	connector->setAvatarProvider(avatarProvider);
	connector->setUserManager(userManager);
	connector->setLichessClient(lichessClient);

	QObject::connect(connector, &ChessConnector::newGameStarted,
		avatarProvider, &AvatarProvider::randomize);

	auto updateLichessToken = [=]() {
		lichessClient->setToken(
			LichessClient::decryptToken(userManager->lichessTokenEncrypted()));
		};
	QObject::connect(userManager, &UserManager::activeUserChanged, [=]() {
		avatarProvider->setPlayerFromUser(userManager->activeUserAvatar());
		updateLichessToken();
		});
	QObject::connect(userManager, &UserManager::lichessChanged, updateLichessToken);

	engine.rootContext()->setContextProperty("chessConnector", connector);
	engine.rootContext()->setContextProperty("avatarProvider", avatarProvider);
	engine.rootContext()->setContextProperty("userManager", userManager);
	engine.rootContext()->setContextProperty("lichessClient", lichessClient);

	engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));

	return app.exec();
}
