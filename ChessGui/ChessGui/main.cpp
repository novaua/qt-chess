#include "stdafx.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QtQml/qqml.h>
#include "chessconnector.h"
#include "applog.h"
#include "frametrace.h"
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QDir>
#include <memory>
#include <windows.h>

void customLogMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
	static const char* typeStr[] = { "DEBUG", "WARN ", "CRIT ", "FATAL", "INFO " };
	AppLog::write(typeStr[type], msg);
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
	// The Online page's "Your games" and Home's Continue (LI-04): fixed rows, no network.
	if (scene == "online" || scene == "continue" || scene == "continue-online" || scene == "continue-list"
	    || scene == "continue-list-online") {
		sceneMap["ongoingGames"] = LichessClient::parseOngoingGames(QJsonDocument::fromJson(R"({ "nowPlaying": [
			{ "gameId": "aEPqtBMc", "color": "white", "isMyTurn": true, "secondsLeft": 540,
			  "fen": "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", "lastMove": "b8c6",
			  "opponent": { "id": "friend", "username": "Friend", "rating": 1650 },
			  "speed": "rapid", "source": "friend", "variant": { "key": "standard" } },
			{ "gameId": "OHp33WmD", "color": "black", "isMyTurn": false, "secondsLeft": 190000,
			  "fen": "rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR", "lastMove": "g8f6",
			  "opponent": { "id": null, "username": "Anonymous" },
			  "speed": "correspondence", "source": "friend", "variant": { "key": "standard" } },
			{ "gameId": "rvaeSdJE", "color": "white", "isMyTurn": true,
			  "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
			  "opponent": { "id": null, "username": "AI", "ai": 3 },
			  "speed": "correspondence", "source": "ai", "variant": { "key": "standard" } },
			{ "gameId": "TiGeHdSm", "color": "black", "isMyTurn": false, "secondsLeft": 150,
			  "fen": "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR", "lastMove": "c7c5",
			  "opponent": { "id": "speedy", "username": "Speedy", "rating": 1890 },
			  "speed": "blitz", "source": "lobby", "variant": { "key": "standard" } }
		] })").object());
	}
	if (!QStringList{ "profiles", "login", "profile", "profile-local", "profile-expired", "import",
	                  "online", "online-empty", "online-blitz", "online-challenge", "continue", "continue-online",
	                  "continue-list", "continue-list-online", "takeback",
	                  "clock-online", "clock-correspondence", "premove", "premove-played", "premove-dropped", "reconnecting", "rejoin",
	                  "challenge", "challenge-unplayable", "challenge-ingame", "online-challenges", "online-challenges-decline",
	                  "draw-offer", "messages", "opponent-left", "opponent-left-claim", "opponent-left-turn",
	                  "opponent-left-error" }.contains(scene))
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

// Scene "takeback" (LI-03 check, not in `all`): a Lichess game fed to the stream parser as the Board
// API sends it: four moves, then the list after an accepted takeback of two. The window is grabbed
// once the board has shown that. No network.
static void playTakebackScene(ChessConnector* connector, LichessClient* lichess)
{
	lichess->setCurrentGameId(QStringLiteral("takeback1"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"takeback1",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 e7e5 g1f3 b8c6","status":"started"}})");
	auto step = std::make_shared<int>(0);
	QObject::connect(connector, &ChessConnector::boardIdle, lichess, [=]() {
		if (++*step == 1)
			lichess->handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started"})");
		else if (*step == 2)
			emit connector->screenshotSceneReady();
	});
}

// Scenes "clock-online" and "clock-correspondence" (LI-02): a Lichess game with its clock, fed to the
// stream parser as the Board API sends it. Time stands still, so the times are the ones sent here.
static void playClockScene(ChessConnector* connector, LichessClient* lichess, const QString& scene)
{
	lichess->setCurrentGameId(QStringLiteral("clock1"));
	lichess->handleGameStreamLine(scene == "clock-online"
		? R"({"type":"gameFull","id":"clock1","clock":{"initial":600000,"increment":5000},
			"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
			"state":{"moves":"e2e4 e7e5 g1f3 b8c6","wtime":431000,"btime":8400,"winc":5000,"binc":5000,"status":"started"}})"
		: R"({"type":"gameFull","id":"clock1","daysPerTurn":3,
			"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
			"state":{"moves":"e2e4 e7e5 g1f3 b8c6","wtime":190800000,"btime":259200000,"winc":0,"binc":0,"status":"started"}})");
	QObject::connect(connector, &ChessConnector::boardIdle, lichess,
		[=]() { emit connector->screenshotSceneReady(); }, Qt::SingleShotConnection);
}

// Scenes "premove", "premove-played" and "premove-dropped" (LI-05): a Lichess game fed to the stream
// parser, White (us) after 1. e4 e5 2. Nf3, Black to move. premove: Bf1-c4 waits, the d2 pawn shows
// where it may go. premove-played: Black's Nc6 comes, then Bc4 is played (not posted: offline).
// premove-dropped: e4-e5 waits, and after Nc6 the pawn can't move there: the premove is gone.
static void playPremoveScene(ChessConnector* connector, LichessClient* lichess, const QString& scene)
{
	lichess->setCurrentGameId(QStringLiteral("premove1"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"premove1",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 e7e5 g1f3","status":"started"}})");
	auto step = std::make_shared<int>(0);
	QObject::connect(connector, &ChessConnector::boardIdle, lichess, [=]() {
		if (++*step == 1) {
			const bool dropped = scene == "premove-dropped";
			connector->figureSelected(dropped ? Chess::e4 : Chess::f1);
			connector->figureSelected(dropped ? Chess::e5 : Chess::c4);
			if (scene == "premove") {
				connector->figureSelected(Chess::d2);
				emit connector->screenshotSceneReady();
			}
			else
				lichess->handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3 b8c6","status":"started"})");
		}
		else if (*step == 2 && scene != "premove")
			emit connector->screenshotSceneReady();
	});
}

// Scene "rejoin" (BL-18): an unfinished Lichess game joined with its moves (as Continue does). Ready
// with the first frame drawn after the join, the game page's first (Take-Screenshot.ps1 grabs this
// scene without a delay): it must show the game's position already, not the start position.
static void playRejoinScene(ChessConnector* connector, LichessClient* lichess, QQuickWindow* window)
{
	lichess->setCurrentGameId(QStringLiteral("rejoin01"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"rejoin01",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6","status":"started"}})");
	if (!window) {
		emit connector->screenshotSceneReady();
		return;
	}
	QObject::connect(window, &QQuickWindow::frameSwapped, connector,
		[=]() { emit connector->screenshotSceneReady(); }, Qt::SingleShotConnection);
	window->update();
}

// Scene "reconnecting" (LI-06): a Lichess game whose connection is lost: the bar over the board. The
// stream parser can't lose a connection it hasn't got, so the scene says it did.
static void playReconnectingScene(ChessConnector* connector, LichessClient* lichess)
{
	lichess->setCurrentGameId(QStringLiteral("reconnect1"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"reconnect1",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 e7e5 g1f3","status":"started"}})");
	QObject::connect(connector, &ChessConnector::boardIdle, lichess, [=]() {
		emit lichess->gameConnectionChanged(false);
		emit connector->screenshotSceneReady();
	}, Qt::SingleShotConnection);
}

// Scenes "draw-offer" and "messages" (BL-13 step 3): a Lichess game whose opponent offers a draw: the
// offer over the command bar (in the InfoBar when it doesn't fit, e.g. 640 wide). "messages" then opens
// the recent messages (ScreenshotScene.qml adds earlier ones).
static void playDrawOfferScene(ChessConnector* connector, LichessClient* lichess, const QString& scene)
{
	lichess->setCurrentGameId(QStringLiteral("draw1"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"draw1",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 e7e5 g1f3","status":"started"}})");
	QObject::connect(connector, &ChessConnector::boardIdle, lichess, [=]() {
		emit lichess->drawOfferReceived();
		// messages: a second more for the list, which opens after the offer and fades in.
		QTimer::singleShot(scene == "messages" ? 1000 : 0, connector, [=]() { emit connector->screenshotSceneReady(); });
	}, Qt::SingleShotConnection);
}

// Scenes "opponent-left", "opponent-left-claim", "opponent-left-turn" and "opponent-left-error": the Lichess
// opponent left the game. The countdown; the win can be claimed (the opponent's turn); the player's turn
// (Lichess takes the claim only on the opponent's turn: a move first); a refused claim's error, which
// takes the command bar's place for its 4 s (the claim waits for an answer: it comes back then).
static void playOpponentLeftScene(ChessConnector* connector, LichessClient* lichess, const QString& scene)
{
	lichess->setCurrentGameId(QStringLiteral("gone1"));
	const QByteArray moves = scene == "opponent-left-turn" ? "e2e4 e7e5" : "e2e4 e7e5 g1f3";
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"gone1",
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":")" + moves + R"(","status":"started"}})");
	QObject::connect(connector, &ChessConnector::boardIdle, lichess, [=]() {
		lichess->handleGameStreamLine(scene == "opponent-left"
			? R"({"type":"opponentGone","gone":true,"claimWinInSeconds":12})"
			: R"({"type":"opponentGone","gone":true,"claimWinInSeconds":0})");
		if (scene == "opponent-left-error")
			emit lichess->networkError(QStringLiteral("Claim victory failed: This is not the time to claim victory"));
		emit connector->screenshotSceneReady();
	}, Qt::SingleShotConnection);
}

// Scenes "challenge", "challenge-unplayable" and "challenge-ingame" (BL-10): challenges to Tester
// (TesterChess on Lichess), fed to the event stream's parser (no network).
static void seedChallenges(LichessClient* lichess, const QString& scene)
{
	const bool online = scene.startsWith("online-challenges");
	if (scene == "challenge-unplayable" || online) {
		lichess->handleEventStreamLine(R"({"type":"challenge","compat":{"bot":false,"board":false},"challenge":{
			"id":"chal0003","url":"https://lichess.org/chal0003","status":"created","direction":"in",
			"challenger":{"id":"speedy","name":"Speedy","rating":2105,"title":"FM"},"destUser":{"id":"testerchess","name":"TesterChess"},
			"variant":{"key":"standard"},"rated":true,"speed":"bullet",
			"timeControl":{"type":"clock","limit":60,"increment":0,"show":"1+0"},"color":"random"}})");
		if (scene == "challenge-unplayable")
			return;
	}
	lichess->handleEventStreamLine(R"({"type":"challenge","compat":{"bot":false,"board":true},"challenge":{
		"id":"chal0001","url":"https://lichess.org/chal0001","status":"created","direction":"in",
		"challenger":{"id":"friendlyrival","name":"FriendlyRival","rating":1712,"provisional":false},"destUser":{"id":"testerchess","name":"TesterChess"},
		"variant":{"key":"standard"},"rated":false,"speed":"rapid",
		"timeControl":{"type":"clock","limit":600,"increment":5,"show":"10+5"},"color":"white"}})");
	if (scene == "challenge" || online)
		lichess->handleEventStreamLine(R"({"type":"challenge","compat":{"bot":false,"board":true},"challenge":{
			"id":"chal0002","url":"https://lichess.org/chal0002","status":"created","direction":"in",
			"challenger":{"id":"slowplayer","name":"SlowPlayer","rating":1540,"provisional":true},"destUser":{"id":"testerchess","name":"TesterChess"},
			"variant":{"key":"standard"},"rated":true,"speed":"correspondence",
			"timeControl":{"type":"correspondence","daysPerTurn":3},"color":"random"}})");
}

// Scene "challenge-ingame": a real-time Lichess game on the board, then a challenge: Decline only.
static void playChallengeInGameScene(ChessConnector* connector, LichessClient* lichess)
{
	lichess->setCurrentGameId(QStringLiteral("ingame01"));
	lichess->handleGameStreamLine(R"({"type":"gameFull","id":"ingame01","clock":{"initial":600000,"increment":5000},
		"white":{"id":"testerchess","name":"TesterChess"},"black":{"id":"friend","name":"Friend"},
		"state":{"moves":"e2e4 e7e5 g1f3","wtime":590000,"btime":596000,"winc":5000,"binc":5000,"status":"started"}})");
	seedChallenges(lichess, QStringLiteral("challenge-ingame"));
	QObject::connect(connector, &ChessConnector::boardIdle, lichess,
		[=]() { emit connector->screenshotSceneReady(); }, Qt::SingleShotConnection);
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
		"welcome", "create-user", "profiles", "home", "continue", "continue-online", "continue-list", "continue-list-online",
		"continue-list-robot", "game", "leave-online", "settings", "about", "restore", "levels",
		"profile", "profile-local", "profile-expired", "import", "online", "online-empty", "online-disconnected",
		"computer", "players", "review", "result", "ended", "moves", "select", "drag", "robot", "login", "menu", "board", "slides",
		"takeback", "clock", "clock-low", "clock-flag", "clock-flag-draw", "clock-promotion", "clock-continue", "stalemate", "players-many", "clock-online", "clock-correspondence", "online-blitz", "online-challenge",
		"premove", "premove-played", "premove-dropped", "premove-robot", "reconnecting", "rejoin",
		"challenge", "challenge-unplayable", "challenge-ingame", "online-challenges", "online-challenges-decline", "captures", "check",
		"draw-offer", "messages", "saved", "home-message", "opponent-left", "opponent-left-claim", "opponent-left-turn",
		"opponent-left-error"
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
	AppLog::open(logPath, screenshotMode);
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
	lichessClient->setOffline(screenshotMode); // the scenes' Lichess token is a placeholder

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
		if (!token.isEmpty()) { // neither in screenshot mode (setOffline)
			lichessClient->refreshAccount();
			lichessClient->refreshOngoingGames(); // LI-04: Continue and the Online page
			lichessClient->startEventStream();    // BL-10: challenges, games started elsewhere
		}
	};
	QObject::connect(userManager, &UserManager::activeUserChanged, [=]() {
		avatarProvider->setPlayerFromUser(userManager->activeUserAvatar());
		updateLichessToken();
		});
	QObject::connect(userManager, &UserManager::lichessChanged, updateLichessToken);
	updateLichessToken(); // load() sets the active user silently — seed the token now
	// The online scene's games (no network in screenshot mode): after the token, which resets the list.
	if (screenshotMode && connector->screenshotScene().contains("ongoingGames"))
		lichessClient->setOngoingGames(connector->screenshotScene().value("ongoingGames").toList());
	if (screenshotMode && (scene == "challenge" || scene == "challenge-unplayable" || scene.startsWith("online-challenges"))) // after the token too
		seedChallenges(lichessClient, scene);
	QObject::connect(lichessClient, &LichessClient::accountFetched, userManager, &UserManager::setLichessAccount);
	QObject::connect(lichessClient, &LichessClient::authExpired, userManager, &UserManager::setLichessExpired);
	// A Lichess game left unfinished (Home, switch user) is now one of the ongoing games (LI-04).
	QObject::connect(connector, &ChessConnector::isOnlineGameChanged, lichessClient, [=]() {
		if (!connector->isOnlineGame())
			lichessClient->refreshOngoingGames();
	});
	QObject::connect(lichessClient, &LichessClient::gameEnded, lichessClient, [=]() {
		QTimer::singleShot(3000, lichessClient, [=]() {
			lichessClient->refreshAccount();      // the new rating
			lichessClient->refreshOngoingGames(); // that game is no longer ongoing
		});
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
	if (screenshotMode && scene == "takeback") // once the scene has logged in (Home is up)
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playTakebackScene(connector, lichessClient); }, Qt::QueuedConnection);
	if (screenshotMode && (scene == "clock-online" || scene == "clock-correspondence"))
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playClockScene(connector, lichessClient, scene); }, Qt::QueuedConnection);
	if (screenshotMode && scene.startsWith("premove") && scene != "premove-robot")
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playPremoveScene(connector, lichessClient, scene); }, Qt::QueuedConnection);
	if (screenshotMode && scene == "challenge-ingame")
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playChallengeInGameScene(connector, lichessClient); }, Qt::QueuedConnection);
	if (screenshotMode && (scene == "draw-offer" || scene == "messages"))
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playDrawOfferScene(connector, lichessClient, scene); }, Qt::QueuedConnection);
	if (screenshotMode && scene.startsWith("opponent-left"))
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playOpponentLeftScene(connector, lichessClient, scene); }, Qt::QueuedConnection);
	if (screenshotMode && scene == "reconnecting")
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=]() { playReconnectingScene(connector, lichessClient); }, Qt::QueuedConnection);
	if (screenshotMode && scene == "rejoin")
		QObject::connect(connector, &ChessConnector::screenshotSceneSetUp, connector,
			[=, &engine]() {
				const auto roots = engine.rootObjects();
				playRejoinScene(connector, lichessClient, roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.first()));
			}, Qt::QueuedConnection);
	if (!engine.rootObjects().isEmpty())
		FrameTrace::watch(qobject_cast<QQuickWindow*>(engine.rootObjects().first()));

	return app.exec();
}
