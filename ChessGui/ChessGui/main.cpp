#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include "chessconnector.h"
#include "AppInfo.h"
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

// Screenshot scenes with several profiles, a linked Lichess account, statistics or backups. No network:
// the token is a placeholder and the account isn't refreshed in screenshot mode.
static void seedScreenshotProfiles(UserManager* um, QVariantMap& sceneMap)
{
	const QString scene = sceneMap.value("scene").toString();
	if (scene == "restore") { // two backups: the profiles before Alice, and now (the same as users.json)
		um->createUser("Bob", "fox");
		um->createUser("Tester", "wizard");
		um->backupNow();
		um->createUser("Alice", "girl");
		um->login(um->users().first().toMap().value("id").toString());
		um->backupNow();
		// Fixed times (the list shows them), so the picture is the same on every run.
		for (const auto& [n, time] : { std::pair{ 1, QTime(9, 30) }, std::pair{ 2, QTime(9, 0) } }) {
			QFile backup(UserManager::backupFilePath(n));
			if (backup.open(QIODevice::ReadWrite))
				backup.setFileTime(QDateTime(QDate::currentDate(), time), QFileDevice::FileModificationTime);
		}
		return;
	}
	if (!QStringList{ "profiles", "login", "profile", "profile-local", "profile-expired", "import" }.contains(scene))
		return;
	um->createUser("Bob", "fox");
	const QString bob = um->activeUserId();
	um->createUser("Alice", "girl");
	const QString alice = um->activeUserId();
	um->createUser("Tester", "wizard");
	const QString me = um->activeUserId();
	if (scene != "profile-local") {
		um->saveLichessCredentials("screenshot-token", "TesterChess");
		um->setLichessAccount("TesterChess", {
			{ "blitz",     QVariantMap{ { "rating", 1612 }, { "games", 214 }, { "prov", false } } },
			{ "rapid",     QVariantMap{ { "rating", 1688 }, { "games", 57 },  { "prov", false } } },
			{ "classical", QVariantMap{ { "rating", 1500 }, { "games", 3 },   { "prov", true } } },
		});
		if (scene == "profile-expired")
			um->setLichessExpired();
	}
	const struct { QString key; QString name; int outcome; } games[] = {
		{ UserManager::computerKey(1), "Robot", 1 }, { UserManager::computerKey(1), "Robot", 1 },
		{ UserManager::computerKey(3), "Robot", 1 }, { UserManager::computerKey(3), "Robot", -1 },
		{ UserManager::computerKey(3), "Robot", 0 }, { UserManager::computerKey(5), "Robot", -1 },
		{ UserManager::userKey(bob), "Bob", 1 },     { UserManager::userKey(bob), "Bob", -1 },
		{ "guest", "Guest", 1 },                     { "lichess", "Lichess", -1 },
	};
	for (const auto& g : games)
		um->recordResult(me, g.outcome, g.key, g.name);
	um->recordResult(bob, 1, UserManager::userKey(alice), "Alice"); // Bob: last played today
	if (scene == "profiles" || scene == "login" || scene == "import")
		um->logout();
	if (scene == "import") {
		const QString file = AppConfig::appDataDir() + "/Bob.chessprofile";
		um->exportProfile(bob, QUrl::fromLocalFile(file));
		sceneMap["importFile"] = QUrl::fromLocalFile(file);
	}
}

int main(int argc, char* argv[])
{
	// Windows 11 look; follows the system light/dark theme and accent colour (Theme.qml can force one).
	QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));

	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");

	// -platform offscreen has no fonts of its own: point it at the Windows ones.
	if (!qEnvironmentVariableIsSet("QT_QPA_FONTDIR"))
		qputenv("QT_QPA_FONTDIR", qgetenv("WINDIR") + "\\Fonts");

	// --screenshot (parsed properly below, after QApplication): 1 screenshot pixel = 1 QML pixel,
	// whatever the monitor's scaling. It has to be set before QApplication exists.
	for (int i = 1; i < argc; ++i) {
		if (qstrcmp(argv[i], "--screenshot") == 0 && !qEnvironmentVariableIsSet("QT_ENABLE_HIGHDPI_SCALING"))
			qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
	}

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/app/pics/app-icon.png")); // made by scripts/make_icons.py, like chess.ico
	if (QGuiApplication::platformName() == "offscreen")
		QGuiApplication::setFont(QFont("Segoe UI", 9)); // the Windows default; offscreen picks any font

	// Screenshot mode, for checking views without clicking through the app:
	//   ChessGui.exe --screenshot menu.png --scene board --moves "e2e4 e7e5"
	// It runs on a fresh temporary profile unless --data-dir is given. The window is real but
	// transparent and ignores input (ChessGame.qml). Not -platform offscreen: it can't force the
	// light/dark colour scheme, so the Fluent controls would get the wrong palette.
	// "login", "menu" and "board" are the older names of "profiles", "home" and "game".
	static const QStringList screenshotScenes = {
		"welcome", "create-user", "profiles", "home", "continue", "game", "settings", "about", "restore", "levels",
		"profile", "profile-local", "profile-expired", "import", "online",
		"computer", "players", "review", "result", "ended", "moves", "select", "drag", "robot", "login", "menu", "board"
	};
	QCommandLineParser parser;
	const QCommandLineOption helpOption = parser.addHelpOption();
	const QCommandLineOption dataDirOption("data-dir",
		"Keep users, settings, saves and logs in <dir> instead of the user's profile.", "dir");
	const QCommandLineOption screenshotOption("screenshot",
		"Open --scene, save a screenshot of the window to <file.png> and exit "
		"(exit code 0 = saved). Log: <file.png>.log", "file.png");
	const QCommandLineOption sceneOption("scene",
		"Screenshot scene: " + screenshotScenes.join(", ") + " (default: home).", "name", "home");
	const QCommandLineOption movesOption("moves",
		"game scene: moves to play first, e.g. \"e2e4 e7e5\".", "moves");
	const QCommandLineOption blackOption("black", "game scene: play Black (board turned around).");
	const QCommandLineOption darkOption("dark", "Screenshot in dark mode.");
	const QCommandLineOption highContrastOption("high-contrast",
		"Screenshot with the app's high-contrast colours (the Fluent controls only follow Windows' own setting).");
	const QCommandLineOption sizeOption("size", "Screenshot window size (default 768x1054).", "WxH");
	const QCommandLineOption delayOption("delay",
		"Milliseconds to wait before the screenshot (default 1000).", "ms", "1000");
	parser.addOptions({ dataDirOption, screenshotOption, sceneOption, movesOption,
		blackOption, darkOption, highContrastOption, sizeOption, delayOption });
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
		: AppInfo::logFolderPath() + "/ChessPlusPlus_AppDebug.log";
	QDir().mkpath(QFileInfo(logPath).absolutePath());
	g_logFile = new QFile(logPath);
	const auto logMode = screenshotMode ? QIODevice::Truncate : QIODevice::Append;
	if (!g_logFile->open(QIODevice::WriteOnly | logMode | QIODevice::Text)) {
		delete g_logFile;
		g_logFile = nullptr;
	}
	qInstallMessageHandler(customLogMessageHandler);

	// The QML singletons below must outlive the engine: children of the app, not of the engine.
	auto* connector = new ChessConnector(&app);
	auto* avatarProvider = new AvatarProvider(&app);
	auto* userManager = new UserManager(&app);
	auto* lichessClient = new LichessClient(&app);
	auto* appInfo = new AppInfo(&app);

	QQmlApplicationEngine engine;

	connector->setAvatarProvider(avatarProvider);
	connector->setUserManager(userManager);
	connector->setLichessClient(lichessClient);

	if (screenshotMode) {
		QVariantMap sceneMap{
			{ "scene", scene },
			{ "moves", parser.value(movesOption) },
			{ "black", parser.isSet(blackOption) },
			{ "dark",  parser.isSet(darkOption) },
			{ "highContrast", parser.isSet(highContrastOption) },
		};
		const QStringList size = parser.value(sizeOption).split('x');
		if (size.size() == 2 && size[0].toInt() > 0 && size[1].toInt() > 0) {
			sceneMap["width"]  = size[0].toInt();
			sceneMap["height"] = size[1].toInt();
		}
		seedScreenshotProfiles(userManager, sceneMap);
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

	// The active profile's Lichess token goes to the client (none while it is expired). A new token:
	// fetch the account for the ratings; a 401 on any call marks the link expired.
	auto lastToken = std::make_shared<QString>();
	auto updateLichessToken = [=]() {
		const QString token = userManager->lichessConnected()
			? LichessClient::decryptToken(userManager->lichessTokenEncrypted()) : QString();
		lichessClient->setUsername(userManager->lichessUsername());
		if (token == *lastToken) return;
		*lastToken = token;
		lichessClient->setToken(token);
		if (!token.isEmpty() && !screenshotMode)
			lichessClient->refreshAccount();
	};
	QObject::connect(userManager, &UserManager::activeUserChanged, [=]() {
		avatarProvider->setPlayerFromUser(userManager->activeUserAvatar());
		updateLichessToken();
		});
	QObject::connect(userManager, &UserManager::lichessChanged, updateLichessToken);
	updateLichessToken(); // load() sets the active user silently — seed the token now
	QObject::connect(lichessClient, &LichessClient::accountFetched, userManager, &UserManager::setLichessAccount);
	QObject::connect(lichessClient, &LichessClient::authExpired, userManager, &UserManager::setLichessExpired);
	QObject::connect(lichessClient, &LichessClient::gameEnded, lichessClient, [=]() {
		QTimer::singleShot(3000, lichessClient, &LichessClient::refreshAccount); // the new rating
	});

	// QML: `import ChessPlusPlus`, then e.g. ChessConnector.startLocalGame(…), UserManager.login(…).
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "ChessConnector", connector);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AvatarProvider", avatarProvider);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "UserManager", userManager);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "LichessClient", lichessClient);
	qmlRegisterSingletonInstance("ChessPlusPlus", 1, 0, "AppInfo", appInfo);

	engine.load(QUrl(QStringLiteral("qrc:/qml/ChessGame.qml")));
	if (screenshotMode && engine.rootObjects().isEmpty())
		return 3;

	return app.exec();
}
