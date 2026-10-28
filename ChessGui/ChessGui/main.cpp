#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "chessconnector.h"
#include "AvatarProvider.h"
#include "UserManager.h"
#include "LichessClient.h"

#include <QQuickStyle>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QDir>
#include <memory>
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
	QQuickStyle::setStyle(QStringLiteral("Basic"));

	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");

	// -platform offscreen has no fonts of its own: point it at the Windows ones.
	if (!qEnvironmentVariableIsSet("QT_QPA_FONTDIR"))
		qputenv("QT_QPA_FONTDIR", qgetenv("WINDIR") + "\\Fonts");

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/app/pics/chess_icon.svg"));
	if (QGuiApplication::platformName() == "offscreen")
		QGuiApplication::setFont(QFont("Segoe UI", 9)); // the Windows default; offscreen picks any font

	// Screenshot mode, for checking views without clicking through the app:
	//   ChessGui.exe -platform offscreen --screenshot menu.png --scene board --moves "e2e4 e7e5"
	// It runs on a fresh temporary profile unless --data-dir is given.
	static const QStringList screenshotScenes = {
		"create-user", "login", "menu", "board", "settings", "levels", "profile"
	};
	QCommandLineParser parser;
	const QCommandLineOption helpOption = parser.addHelpOption();
	const QCommandLineOption dataDirOption("data-dir",
		"Keep users, settings, saves and logs in <dir> instead of the user's profile.", "dir");
	const QCommandLineOption screenshotOption("screenshot",
		"Open --scene, save a screenshot of the window to <file.png> and exit "
		"(exit code 0 = saved). Log: <file.png>.log", "file.png");
	const QCommandLineOption sceneOption("scene",
		"Screenshot scene: " + screenshotScenes.join(", ") + " (default: menu).", "name", "menu");
	const QCommandLineOption movesOption("moves",
		"board scene: moves to play first, e.g. \"e2e4 e7e5\".", "moves");
	const QCommandLineOption blackOption("black", "board scene: play Black (board turned around).");
	const QCommandLineOption darkOption("dark", "Screenshot in dark mode.");
	const QCommandLineOption sizeOption("size", "Screenshot window size (default 768x1054).", "WxH");
	const QCommandLineOption delayOption("delay",
		"Milliseconds to wait before the screenshot (default 1000).", "ms", "1000");
	parser.addOptions({ dataDirOption, screenshotOption, sceneOption, movesOption,
		blackOption, darkOption, sizeOption, delayOption });
	// parse(), not process(): its errors would open a message box and block a scripted run.
	if (!parser.parse(app.arguments())) {
		OutputDebugStringW(reinterpret_cast<const WCHAR*>((parser.errorText() + u'\n').utf16()));
		return 1;
	}
	if (parser.isSet(helpOption))
		parser.showHelp();

	const bool screenshotMode = parser.isSet(screenshotOption);
	const QString screenshotPath = QFileInfo(parser.value(screenshotOption)).absoluteFilePath();
	const QString scene = parser.value(sceneOption);
	if (screenshotMode && !screenshotScenes.contains(scene))
		return 4;

	std::unique_ptr<QTemporaryDir> tempDataDir;
	QString dataDir = parser.value(dataDirOption);
	if (dataDir.isEmpty() && screenshotMode) {
		tempDataDir = std::make_unique<QTemporaryDir>();
		dataDir = tempDataDir->path();
	}
	if (!dataDir.isEmpty()) {
		dataDir = QFileInfo(dataDir).absoluteFilePath();
		AppConfig::setAppDataDir(dataDir);
		// QML Settings (window geometry, dark mode, sound) go to an ini file there instead of the registry.
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dataDir);
	}

	const QString logPath = screenshotMode ? screenshotPath + ".log"
		: AppConfig::appDataDir() + "/logs/ChessPlusPlus_AppDebug.log";
	QDir().mkpath(QFileInfo(logPath).absolutePath());
	g_logFile = new QFile(logPath);
	const auto logMode = screenshotMode ? QIODevice::Truncate : QIODevice::Append;
	if (!g_logFile->open(QIODevice::WriteOnly | logMode | QIODevice::Text)) {
		delete g_logFile;
		g_logFile = nullptr;
	}
	qInstallMessageHandler(customLogMessageHandler);

	QQmlApplicationEngine engine;

	auto* connector = new ChessConnector(&engine);
	auto* avatarProvider = new AvatarProvider(&engine);
	auto* userManager = new UserManager(&engine);
	auto* lichessClient = new LichessClient(&engine);

	connector->setAvatarProvider(avatarProvider);
	connector->setUserManager(userManager);
	connector->setLichessClient(lichessClient);

	if (screenshotMode) {
		QVariantMap sceneMap{
			{ "scene", scene },
			{ "moves", parser.value(movesOption) },
			{ "black", parser.isSet(blackOption) },
			{ "dark",  parser.isSet(darkOption) },
		};
		const QStringList size = parser.value(sizeOption).split('x');
		if (size.size() == 2 && size[0].toInt() > 0 && size[1].toInt() > 0) {
			sceneMap["width"]  = size[0].toInt();
			sceneMap["height"] = size[1].toInt();
		}
		connector->setScreenshotScene(sceneMap);

		const int delay = qMax(0, parser.value(delayOption).toInt());
		QObject::connect(connector, &ChessConnector::screenshotSceneReady, &app, [&engine, screenshotPath, delay]() {
			QTimer::singleShot(delay, &engine, [&engine, screenshotPath]() {
				const auto roots = engine.rootObjects();
				auto* window = roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.first());
				const bool ok = window && window->grabWindow().save(screenshotPath);
				qInfo() << "Screenshot" << screenshotPath << (ok ? "saved" : "FAILED");
				QCoreApplication::exit(ok ? 0 : 1);
			});
		});
		// The scene never became ready (e.g. a QML error): don't hang a scripted run.
		QTimer::singleShot(delay + 30000, &app, []() {
			qWarning() << "Screenshot: timed out waiting for the scene";
			QCoreApplication::exit(2);
		});
	}

	QObject::connect(connector, &ChessConnector::newGameStarted,
		avatarProvider, &AvatarProvider::randomize);

	auto updateLichessToken = [=]() {
		lichessClient->setToken(LichessClient::decryptToken(userManager->lichessTokenEncrypted()));
		lichessClient->setUsername(userManager->lichessUsername());
	};
	QObject::connect(userManager, &UserManager::activeUserChanged, [=]() {
		avatarProvider->setPlayerFromUser(userManager->activeUserAvatar());
		updateLichessToken();
		});
	QObject::connect(userManager, &UserManager::lichessChanged, updateLichessToken);
	updateLichessToken(); // load() sets the active user silently — seed the token now

	engine.rootContext()->setContextProperty("chessConnector", connector);
	engine.rootContext()->setContextProperty("avatarProvider", avatarProvider);
	engine.rootContext()->setContextProperty("userManager", userManager);
	engine.rootContext()->setContextProperty("lichessClient", lichessClient);

	engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));
	if (screenshotMode && engine.rootObjects().isEmpty())
		return 3;

	return app.exec();
}
