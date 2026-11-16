#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <atomic>
#include <memory>
#include "ChessEnginePlayer.h"

class GameLibrary;
class QThread;

// Runs on its own QThread (as EngineWorker): its UciConnector, at full strength, is made and used only
// there. It gets a copy of the moves and reports every position as it is done.
class AnalysisWorker : public QObject
{
	Q_OBJECT
	std::shared_ptr<UciConnector> _connector;
	// stop() runs on the GUI thread while analyse() may be starting the engine here (EngineWorker's way).
	std::atomic<bool> _stopped{ false };
	std::atomic<UciConnector*> _connectorForKill{ nullptr };

public:
	// Positions 0…n of the game with `history`: each searched to `depth` and reported (White's view).
	// The last one, when the game is over there (mate, stalemate), isn't searched: `finalEval` is its
	// score then (hasFinal).
	void analyse(int request, const Chess::MovesHistory& history, int depth, Chess::PositionMode mode,
	             bool hasFinal, int finalEval);
	void stop(); // any thread: the search ends (the engine process is terminated), nothing more is reported

signals:
	void positionDone(int request, int position, int evalWhite, QString bestMove);
	void finished(int request);
	void failed(int request, QString message);
};

// The engine's analysis of a kept game (BL-19 step 6): every position searched to `depth` on a thread of
// its own, the progress as it goes, the result kept on the game's record (GameLibrary::setAnalysis) when
// the moves are still the same. One at a time: a new start stops the one running. ChessConnector owns one
// and gives QML its state (analyseGame, cancelAnalysis, Analysing…).
class GameAnalyzer : public QObject
{
	Q_OBJECT

public:
	static constexpr int DefaultDepth = 14;
	static constexpr int MinDepth = 6, MaxDepth = 30;

	explicit GameAnalyzer(QObject* parent = nullptr);
	~GameAnalyzer() override; // stops a running analysis and waits for its thread (3 s at most)

	void setLibrary(GameLibrary* library);
	void setPositionMode(Chess::PositionMode mode) { _mode = mode; }
	int  depth() const { return _depth; }
	void setDepth(int depth); // MinDepth…MaxDepth; the next analysis uses it

	bool    running() const { return _worker != nullptr; }
	QString gameId() const { return _gameId; }  // the game being analysed (or the last one)
	int     done() const { return _evals.size(); } // positions searched so far
	int     total() const { return _total; }       // positions to search: the moves + 1

	// Starts analysing a kept game; one already running stops first. False (nothing started) when there
	// is no such game, it has no move, or its moves can't be played.
	bool start(const QString& gameId);
	void cancel(); // nothing is kept; no signal but runningChanged

signals:
	void runningChanged();
	void progressChanged();
	void finished(const QString& gameId);                          // kept on the record
	void failed(const QString& gameId, const QString& message);    // the engine failed, or the game changed

private:
	void onPositionDone(int request, int position, int evalWhite, const QString& bestMove);
	void onFinished(int request);
	void onFailed(int request, const QString& message);
	void onLibraryChanged();
	void stopThread();
	void fail(const QString& message);

	QPointer<GameLibrary> _library;
	Chess::PositionMode _mode = Chess::PositionMode::FenWindow;
	int _depth = DefaultDepth;

	int _request = 0;
	QThread* _thread = nullptr;
	AnalysisWorker* _worker = nullptr;
	QString _gameId;
	QString _userId;     // the profile the game is of: another profile's library hasn't it
	QStringList _moves;  // the moves analysed: kept only if the record still has them
	int _runDepth = 0;
	int _total = 0;
	QList<int> _evals;
	QStringList _best;
};
