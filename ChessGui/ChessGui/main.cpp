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

// Custom message handler function
void customLogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	// Define your log directory and file name
	// This saves to C:/Users/<User>/AppData/Local/<YourAppName>/logs/app_debug.log
	QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QString logDirPath = appDataPath + "/logs";

	QDir logDir(logDirPath);
	if (!logDir.exists()) {
		logDir.mkpath(".");
	}

	QString logFilePath = logDirPath + "/ChessPlusPlus_AppDebug.log";
	QFile logFile(logFilePath);

	// Open file in Append mode
	if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		QTextStream stream(&logFile);
		QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

		// Format the type text
		QString typeStr;
		switch (type) {
		case QtDebugMsg:    typeStr = "DEBUG"; break;
		case QtWarningMsg:  typeStr = "WARN "; break;
		case QtCriticalMsg: typeStr = "CRIT "; break;
		case QtFatalMsg:    typeStr = "FATAL"; break;
		case QtInfoMsg:     typeStr = "INFO "; break;
		}

		// Write formatted entry to the file
		stream << "[" << timestamp << "] [" << typeStr << "] " << msg << "\n";
		logFile.close();
	}

	// Still send output to Visual Studio Debug Window so you can see it live
	QString vsMsg = QString("%1\n").arg(msg);
	OutputDebugStringW(reinterpret_cast<const WCHAR*>(vsMsg.utf16()));
}

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");

	// Install the file-logging handler before the application starts
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

	// Test the output file
	qDebug() << "Log file system initialized successfully!";
	return app.exec();
}
