#include "stdafx.h"
#include "gameanalyzer.h"

#include "gameanalysis.h"
#include "gamelibrary.h"

#include <QDeadlineTimer>
#include <QDebug>
#include <QThread>
#include <algorithm>

// ── AnalysisWorker (the analysis thread) ─────────────────────────────────────

void AnalysisWorker::stop()
{
	_stopped.store(true);
	if (auto* connector = _connectorForKill.load())
		connector->Kill(); // thread-safe: terminates the process, never touches QProcess
}

void AnalysisWorker::analyse(int request, const Chess::MovesHistory& history, int depth, Chess::PositionMode mode,
                             bool hasFinal, int finalEval, const Chess::StartPosition& start)
{
	if (_stopped.load())
		return;
	try {
		if (!_connector) {
			_connector = std::make_shared<UciConnector>();
			_connectorForKill.store(_connector.get());
		}
		_connector->Init(); // at full strength: a new connector never limits it
		if (_stopped.load())
			return;
		_connector->SetChess960(start.IsChess960());
		_connector->NewGame();
		const int plies = (int)history.size();
		for (int position = 0; position <= plies; ++position) {
			if (_stopped.load())
				return;
			if (position == plies && hasFinal) {
				emit positionDone(request, position, finalEval, QString());
				break;
			}
			const auto candidates = _connector->GetCandidateMoves(
				Chess::ChessEnginePlayer::MakeRequest(history, position, mode, start), 1, depth);
			const auto& best = candidates.front();
			const bool whiteToMove = position % 2 == 0;
			const QString move = best.Move == "(none)" ? QString() : QString::fromStdString(best.Move);
			emit positionDone(request, position, whiteToMove ? best.ScoreCp : -best.ScoreCp, move);
		}
		if (!_stopped.load())
			emit finished(request);
	} catch (const std::exception& e) {
		if (!_stopped.load()) // else the error is from the process we just terminated
			emit failed(request, QString::fromStdString(e.what()));
	}
}

// ── GameAnalyzer ─────────────────────────────────────────────────────────────

GameAnalyzer::GameAnalyzer(QObject* parent)
	: QObject(parent)
{
}

// The app's exit: a running QThread must not be destroyed. The stopped analyses' threads are children
// here until they finish; a stop ends even the engine's start at once, so they are done in a few ms.
GameAnalyzer::~GameAnalyzer()
{
	stopThread();
	const QDeadlineTimer deadline(3000);
	for (QThread* thread : findChildren<QThread*>(Qt::FindDirectChildrenOnly)) {
		if (thread->wait(deadline))
			continue;
		qWarning() << "Analysis thread did not stop within 3 s; leaving it behind";
		thread->setParent(nullptr);
	}
}

void GameAnalyzer::setLibrary(GameLibrary* library)
{
	if (_library)
		disconnect(_library, nullptr, this, nullptr);
	_library = library;
	if (_library)
		connect(_library, &GameLibrary::gamesChanged, this, &GameAnalyzer::onLibraryChanged);
}

void GameAnalyzer::setDepth(int depth)
{
	_depth = std::clamp(depth, MinDepth, MaxDepth);
}

bool GameAnalyzer::start(const QString& gameId)
{
	if (!_library)
		return false;
	const auto record = _library->find(gameId);
	if (!record || record->moves.isEmpty())
		return false;
	const auto game = record->replay();
	if (!game)
		return false; // a damaged record

	const QString interruptedId = running() && _gameId != gameId ? _gameId : QString();
	stopThread();
	_gameId = gameId;
	_userId = _library->userId();
	_moves = record->moves;
	_runDepth = _depth;
	_total = (int)_moves.size() + 1;
	_evals.clear();
	_best.clear();

	// The last position, when the game is over there, isn't for the engine: the side to move is mated
	// (the winner's mate in 0) or stalemated.
	bool hasFinal = false;
	int finalEval = 0;
	if (game->IsCheckMate()) {
		hasFinal = true;
		finalEval = record->whiteToMove() ? -GameAnalysis::MateCp : GameAnalysis::MateCp;
	} else if (game->IsStalemate()) {
		hasFinal = true;
	}

	auto* worker = new AnalysisWorker;
	_thread = new QThread(this);
	_worker = worker;
	worker->moveToThread(_thread);
	connect(worker, &AnalysisWorker::positionDone, this, &GameAnalyzer::onPositionDone);
	connect(worker, &AnalysisWorker::finished, this, &GameAnalyzer::onFinished);
	connect(worker, &AnalysisWorker::failed, this, &GameAnalyzer::onFailed);
	connect(_thread, &QThread::finished, worker, &QObject::deleteLater); // on its own thread (its QProcess)
	_thread->start();

	const int request = ++_request;
	const auto history = game->GetGameRecord();
	const auto start = game->GetStartPosition(); // the record's set-up (Chess960, LI-01A)
	const int depth = _runDepth;
	const auto mode = _mode;
	QMetaObject::invokeMethod(worker, [=] { worker->analyse(request, history, depth, mode, hasFinal, finalEval, start); },
		Qt::QueuedConnection);
	emit runningChanged();
	emit progressChanged();
	if (!interruptedId.isEmpty())
		emit interrupted(interruptedId);
	return true;
}

void GameAnalyzer::cancel()
{
	if (!running())
		return;
	stopThread();
	emit runningChanged();
}

// Never waits (as ChessConnector::stopEngineThread): the worker is deleted on its thread once that
// finishes, then the thread here.
void GameAnalyzer::stopThread()
{
	if (!_thread)
		return;
	++_request; // what is still on its way belongs to the analysis being stopped
	disconnect(_worker, nullptr, this, nullptr);
	QThread* thread = _thread;
	connect(thread, &QThread::finished, thread, &QObject::deleteLater);
	_worker->stop();
	thread->quit();
	_thread = nullptr;
	_worker = nullptr;
}

void GameAnalyzer::onPositionDone(int request, int position, int evalWhite, const QString& bestMove)
{
	if (request != _request || position != _evals.size())
		return;
	_evals.append(evalWhite);
	_best.append(bestMove);
	emit progressChanged();
}

void GameAnalyzer::onFinished(int request)
{
	if (request != _request)
		return;
	stopThread();
	emit runningChanged();
	const GameAnalysis analysis(_runDepth, _evals, _best, _moves);
	if (_library && _library->userId() == _userId && _library->setAnalysis(_gameId, _moves, analysis))
		emit finished(_gameId);
	else
		emit failed(_gameId, QStringLiteral("The game changed while it was analysed, so its analysis wasn't kept."));
}

void GameAnalyzer::onFailed(int request, const QString& message)
{
	if (request != _request)
		return;
	qWarning().noquote() << "Analysis of" << _gameId << "failed:" << message;
	fail(QStringLiteral("The chess engine couldn't analyse the game."));
}

// The game's moves changed (a takeback), it was deleted, or the profile changed: its analysis would be
// of another game, or of none. The player did that: it stops without a word (no failure to report).
void GameAnalyzer::onLibraryChanged()
{
	if (!running() || !_library)
		return;
	const auto record = _library->find(_gameId);
	if (_library->userId() != _userId || !record || record->moves != _moves) {
		qInfo().noquote() << "Analysis of" << _gameId << "stopped: the game changed, was deleted or the profile changed";
		cancel();
	}
}

void GameAnalyzer::fail(const QString& message)
{
	stopThread();
	emit runningChanged();
	emit failed(_gameId, message);
}
