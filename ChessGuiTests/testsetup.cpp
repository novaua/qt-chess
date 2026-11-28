#include "testsetup.h"

#include "AppConfig.h"
#include "appservices.h"
#include "fakelichessserver.h"
#include "gamelibrary.h"
#include "OnlineClient.h"
#include "UserManager.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QtQml/qqml.h>
#include <cstdlib>

TestSetup::TestSetup()
	: _resultsDir(qEnvironmentVariable("CHESS_UI_TEST_RESULTS"))
	, _stepDelay(qMax(0, qEnvironmentVariableIntValue("CHESS_UI_TEST_STEP_MS")))
	, _watched(qEnvironmentVariableIntValue("CHESS_UI_TEST_WATCHED") != 0)
	, _run(qMax(1, qEnvironmentVariableIntValue("CHESS_UI_TEST_RUN")))
{
	// A test file that runs twice (a restart) gets its folder from Run-UiTests.ps1, the same for both.
	_dataDir = qEnvironmentVariable("CHESS_UI_TEST_DATA_DIR");
	if (_dataDir.isEmpty())
		_dataDir = _tempDataDir.path();
	else
		QDir().mkpath(_dataDir);
}
TestSetup::~TestSetup() = default;

// Before the first test file: a fresh profile folder (users, settings, saves), as --data-dir does,
// then the services, which read it.
void TestSetup::applicationAvailable()
{
	// The robot is FakeUci, built next to the tests: never Stockfish, which may not be there (GitHub)
	// and doesn't play the same moves twice. An empty one counts as none (BL-37: a shell can hold it empty,
	// and the app then looks for its own engine, Stockfish).
	if (qEnvironmentVariableIsEmpty("CHESS_ENGINE_PATH"))
		qputenv("CHESS_ENGINE_PATH", QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/FakeUci.exe").toUtf8());
	setEngine({});

	// BL-39: the tests' compiled QML apart from the app's (the same names): never rewritten under a running app.
	_qmlCachePath = AppConfig::useOwnQmlCache();

	AppConfig::setAppDataDir(_dataDir);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _dataDir);

	// Children of the app: the QML singletons outlive the test file's engine, as in main.cpp.
	// Lichess is FakeLichess on 127.0.0.1 (BL-05 step 4c): online, but nothing reaches lichess.org,
	// and without linkLichess() there is no token, so nothing is asked of it at all.
	// LI-18: the app's plugins (online\ beside the tests), or none in a file run without them
	// ("// ui-test-online: none": Run-UiTests.ps1 sets CHESS_UI_TEST_ONLINE=none).
	const bool online = qEnvironmentVariable("CHESS_UI_TEST_ONLINE") != QLatin1String("none");
	const QString pluginDir = online ? AppServices::defaultPluginDir() : _dataDir + QStringLiteral("/no-plugins");
	_lichess = new FakeLichessServer(QCoreApplication::instance());
	_services = std::make_unique<AppServices>(QCoreApplication::instance(), false, pluginDir);
	if (OnlineClient* lichess = _services->lichessAccountClient()) {
		lichess->setBaseUrl(_lichess->url());
		lichess->setAssetBaseUrl(_lichess->url()); // LI-11: the flairs' images too
		// A lost stream is tried again after 0.2 s (then 0.4, 0.8, …), a move without an answer gives up
		// after 3 s; FakeLichess's keep-alive comes every second, well inside the 10 s of silence.
		lichess->setStreamTiming(10000, 200, 3000, 1000, 2000);
	} else if (online) {
		qCritical() << "TestSetup: no Lichess plugin in" << pluginDir;
	}
	_services->start();
	_services->registerQmlTypes();
	qmlRegisterSingletonInstance("ChessGuiTests", 1, 0, "TestRun", this);
	qmlRegisterSingletonInstance("ChessGuiTests", 1, 0, "FakeLichess", _lichess);
}

// The engine process gets this process's environment when it starts.
void TestSetup::useFakeEngine()
{
	qputenv("CHESS_ENGINE_PATH", QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/FakeUci.exe").toUtf8());
}

void TestSetup::setEngine(const QString& moves, int delayMs)
{
	qputenv("FAKEUCI_MOVES", moves.toUtf8());
	qputenv("FAKEUCI_DELAY_MS", QByteArray::number(qMax(0, delayMs)));
}

void TestSetup::linkLichess(const QString& username)
{
	_services->userManager()->saveLichessCredentials(QStringLiteral("fake-token"), username);
}

void TestSetup::addFinishedGames(int count)
{
	const QDateTime now = QDateTime::currentDateTime();
	for (int i = 0; i < count; ++i) {
		GameRecord r;
		r.kind = QStringLiteral("local");
		r.opponentName = QStringLiteral("Guest");
		r.setMoves({ "f2f3", "e7e5", "g2g4", "d8h4" });
		r.startedAt = now.addDays(-30 - i);
		r.finishedAt = r.startedAt.addSecs(60);
		r.finished = true;
		r.result = QStringLiteral("0-1");
		r.resultText = QStringLiteral("Guest wins by checkmate");
		r.resultRecorded = true;
		_services->gameLibrary()->put(r);
	}
}

QString TestSetup::addLichessGame()
{
	GameRecord r;
	r.id = GameRecord::lichessRecordId(QStringLiteral("kept0001"));
	r.kind = QStringLiteral("lichess");
	r.opponentName = QStringLiteral("Magnus");
	r.setMoves({ "e2e4", "e7e5", "g1f3", "b8c6", "f1b5" });
	r.startedAt = QDateTime(QDate(2026, 11, 20), QTime(12, 0));
	r.finishedAt = r.startedAt.addSecs(900);
	r.finished = true;
	r.result = QStringLiteral("1-0");
	r.resultText = QStringLiteral("Magnus resigned");
	r.resultRecorded = true;
	r.clockLimitSec = 600;
	r.clockIncSec = 5;
	r.lichess = LichessGameInfo(QStringLiteral("kept0001"), true, QStringLiteral("rapid"), 0,
		OnlinePlayer::fromJson(QJsonObject{ { "id", "testerchess" }, { "name", "TesterChess" }, { "rating", 1650 } }),
		OnlinePlayer::fromJson(QJsonObject{ { "id", "magnus" }, { "name", "Magnus" }, { "title", "GM" }, { "rating", 2850 } }));
	return _services->gameLibrary()->put(r);
}

QString TestSetup::addDamagedGame()
{
	GameRecord r;
	r.kind = QStringLiteral("local");
	r.opponentName = QStringLiteral("Guest");
	r.moves = QStringList{ "e2e4", "e7e5", "e4e6", "g8f6" }; // e4-e6: the pawn on e5 is in the way
	r.startedAt = QDateTime::currentDateTime().addDays(-60);
	return _services->gameLibrary()->put(r);
}

void TestSetup::editRatingFile()
{
	QFile file(GameLibrary::ratingFilePathOf(_services->gameLibrary()->userId()));
	if (!file.open(QIODevice::WriteOnly))
		qCritical() << "TestSetup: can't write" << file.fileName();
	file.write(R"({"version":1,"rating":{"games":50,"value":2400},"sig":"1:00"})");
}

QString TestSetup::addEditedGame()
{
	GameLibrary* library = _services->gameLibrary();
	GameRecord r;
	r.kind = QStringLiteral("local");
	r.opponentName = QStringLiteral("Guest");
	r.setMoves({ "f2f3", "e7e5", "g2g4", "d8h4" });
	r.startedAt = QDateTime::currentDateTime().addDays(-60);
	r.finishedAt = r.startedAt.addSecs(60);
	r.finished = true;
	r.result = QStringLiteral("0-1");
	r.resultText = QStringLiteral("Guest wins by checkmate");
	const QString id = library->put(r);
	library->flush();
	QFile file(GameLibrary::gameFilePathOf(library->userId(), id));
	if (!file.open(QIODevice::ReadWrite))
		qCritical() << "TestSetup: can't edit" << file.fileName();
	QByteArray content = file.readAll();
	content.replace("\"result\":\"0-1\"", "\"result\":\"1-0\"");
	file.resize(0);
	file.write(content);
	return id;
}

QUrl TestSetup::fileUrl(const QString& name) const
{
	return QUrl::fromLocalFile(QDir(_dataDir).filePath(name));
}

QString TestSetup::readTextFile(const QUrl& file) const
{
	QFile f(file.toLocalFile());
	return f.open(QIODevice::ReadOnly | QIODevice::Text) ? QString::fromUtf8(f.readAll()) : QString();
}

QString TestSetup::clipboardText() const
{
	return QGuiApplication::clipboard()->text();
}

QStringList TestSetup::filesIn(const QString& folder) const
{
	return QDir(folder).entryList(QDir::Files);
}

// The singletons are global, so a second test file in this process would see the first one's
// profile and game: stop, instead of tests that pass or fail by the order they ran in.
void TestSetup::qmlEngineAvailable(QQmlEngine*)
{
	if (++_engines == 1)
		return;
	qCritical() << "ChessGuiTests: one test file per process: -input qml\\tst_<name>.qml"
	               " (scripts\\Run-UiTests.ps1 runs them all that way).";
	std::exit(3);
}
