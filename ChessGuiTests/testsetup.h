#pragma once

#include <QObject>
#include <QTemporaryDir>
#include <memory>

class AppServices;
class QQmlEngine;

// Qt Quick Test's setup object (BL-05): the app's services on a fresh data folder, made by the
// app's own AppServices, so a test runs what main.cpp runs. One test file per process: the
// services and their profile are the file's own (scripts\Run-UiTests.ps1 runs them one by one).
// Also the run's options for the tests, QML singleton `TestRun` (`import ChessGuiTests`), from the
// environment Run-UiTests.ps1 sets.
class TestSetup : public QObject
{
	Q_OBJECT
	// Where a failed test function leaves its picture: <resultsDir>/<file>-<function>.png ("": nowhere)
	Q_PROPERTY(QString resultsDir READ resultsDir CONSTANT)
	// A pause after each action, to watch a run in a real window (-Visible), ms
	Q_PROPERTY(int stepDelay READ stepDelay CONSTANT)
	// After a failure the window stays up until it is closed (-Visible)
	Q_PROPERTY(bool pauseOnFailure READ pauseOnFailure CONSTANT)

public:
	TestSetup();
	~TestSetup() override;

	QString resultsDir() const { return _resultsDir; }
	int stepDelay() const { return _stepDelay; }
	bool pauseOnFailure() const { return _pauseOnFailure; }

public slots:
	void applicationAvailable();
	void qmlEngineAvailable(QQmlEngine* engine);

private:
	QTemporaryDir _dataDir;
	std::unique_ptr<AppServices> _services;
	int _engines = 0;
	QString _resultsDir;
	int _stepDelay = 0;
	bool _pauseOnFailure = false;
};
