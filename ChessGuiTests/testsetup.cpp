#include "testsetup.h"

#include "AppConfig.h"
#include "appservices.h"
#include "fakelichessserver.h"
#include "LichessClient.h"
#include "UserManager.h"

#include <QCoreApplication>
#include <QDir>
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
	// and doesn't play the same moves twice.
	if (!qEnvironmentVariableIsSet("CHESS_ENGINE_PATH"))
		qputenv("CHESS_ENGINE_PATH", QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/FakeUci.exe").toUtf8());
	setEngine({});

	AppConfig::setAppDataDir(_dataDir);
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _dataDir);

	// Children of the app: the QML singletons outlive the test file's engine, as in main.cpp.
	// Lichess is FakeLichess on 127.0.0.1 (BL-05 step 4c): online, but nothing reaches lichess.org,
	// and without linkLichess() there is no token, so nothing is asked of it at all.
	_lichess = new FakeLichessServer(QCoreApplication::instance());
	_services = std::make_unique<AppServices>(QCoreApplication::instance(), false);
	_services->lichessClient()->setBaseUrl(_lichess->url());
	// A lost stream is tried again after 0.2 s (then 0.4, 0.8, …), a move without an answer gives up
	// after 3 s; FakeLichess's keep-alive comes every second, well inside the 10 s of silence.
	_services->lichessClient()->setStreamTiming(10000, 200, 3000, 1000, 2000);
	_services->start();
	_services->registerQmlTypes();
	qmlRegisterSingletonInstance("ChessGuiTests", 1, 0, "TestRun", this);
	qmlRegisterSingletonInstance("ChessGuiTests", 1, 0, "FakeLichess", _lichess);
}

// The engine process gets this process's environment when it starts.
void TestSetup::setEngine(const QString& moves, int delayMs)
{
	qputenv("FAKEUCI_MOVES", moves.toUtf8());
	qputenv("FAKEUCI_DELAY_MS", QByteArray::number(qMax(0, delayMs)));
}

void TestSetup::linkLichess(const QString& username)
{
	_services->userManager()->saveLichessCredentials(QStringLiteral("fake-token"), username);
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
