#pragma once

#include <QObject>
#include <QTemporaryDir>
#include <QUrl>
#include <memory>

class AppServices;
class FakeLichessServer;
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
	// Watched (-Visible): the window on the screen, centred, and after a failure it stays up until closed
	Q_PROPERTY(bool watched READ watched CONSTANT)
	// Which run of a test file that runs more than once on the same data folder ("// ui-test-runs: 2":
	// a restart), from 1
	Q_PROPERTY(int run READ run CONSTANT)

public:
	TestSetup();
	~TestSetup() override;

	QString resultsDir() const { return _resultsDir; }
	int stepDelay() const { return _stepDelay; }
	bool watched() const { return _watched; }
	int run() const { return _run; }

	// The robot of the next game (FakeUci, read when its engine starts): the game it follows, in UCI
	// ("e2e4 e7e5 …", it plays its own half), and how long it thinks before each answer.
	Q_INVOKABLE void setEngine(const QString& moves, int delayMs = 0);
	// The active profile linked to Lichess as `username` (FakeLichess's player), as the sign-in ends:
	// the account, the ongoing games and the event stream follow, from FakeLichess. Lichess's sign-in
	// page itself (LichessAuth) is lichess.org's: not in a test.
	Q_INVOKABLE void linkLichess(const QString& username);
	// BL-19: that many finished two-player games (Fool's mate, vs Guest) in the active profile's library,
	// a day apart and older than any game played in the test: many games without playing them.
	Q_INVOKABLE void addFinishedGames(int count);
	// An unfinished two-player game whose third move can't be played (a damaged record); its id.
	Q_INVOKABLE QString addDamagedGame();
	// BL-20: a finished two-player game whose file is then edited on disk (its result turned round), as a
	// player might: the next start sets it aside. Its id.
	Q_INVOKABLE QString addEditedGame();
	// BL-20 step 2: the active profile's rating.json written by hand (2400 after 50 games, a made-up
	// signature): the next start restores the rating (no rated game kept: it starts again).
	Q_INVOKABLE void editRatingFile();
	// A file in the test's data folder, as a URL (e.g. for Export PGN), and a text file's contents.
	Q_INVOKABLE QUrl fileUrl(const QString& name) const;
	Q_INVOKABLE QString readTextFile(const QUrl& file) const;

public slots:
	void applicationAvailable();
	void qmlEngineAvailable(QQmlEngine* engine);

private:
	QTemporaryDir _tempDataDir;
	QString _dataDir; // the temporary one, or CHESS_UI_TEST_DATA_DIR (kept from one run to the next)
	std::unique_ptr<AppServices> _services;
	FakeLichessServer* _lichess = nullptr; // the app's Lichess in every test file: never lichess.org
	int _engines = 0;
	QString _resultsDir;
	int _stepDelay = 0;
	bool _watched = false;
	int _run = 1;
};
