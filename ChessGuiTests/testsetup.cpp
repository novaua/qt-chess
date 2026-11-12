#include "testsetup.h"

#include "AppConfig.h"
#include "appservices.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QtQml/qqml.h>
#include <cstdlib>

TestSetup::TestSetup()
	: _resultsDir(qEnvironmentVariable("CHESS_UI_TEST_RESULTS"))
	, _stepDelay(qMax(0, qEnvironmentVariableIntValue("CHESS_UI_TEST_STEP_MS")))
	, _watched(qEnvironmentVariableIntValue("CHESS_UI_TEST_WATCHED") != 0)
{
}
TestSetup::~TestSetup() = default;

// Before the first test file: a fresh profile folder (users, settings, saves), as --data-dir does,
// then the services, which read it.
void TestSetup::applicationAvailable()
{
	AppConfig::setAppDataDir(_dataDir.path());
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _dataDir.path());

	// Children of the app: the QML singletons outlive the test file's engine, as in main.cpp.
	_services = std::make_unique<AppServices>(QCoreApplication::instance(), true);
	_services->start();
	_services->registerQmlTypes();
	qmlRegisterSingletonInstance("ChessGuiTests", 1, 0, "TestRun", this);
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
