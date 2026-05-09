#include "stdafx.h"
#include "chessconnector.h"
#include "ChessException.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QThread>

using namespace Chess;

const char* EmptyFlag = " ";
const char* DefaultSaveGameFile = "chess.save";

void ClearBoard(QStringList& board, const QString& cleanValue = EmptyFlag)
{
	auto newBoard = board.empty();
	for (auto i = 0; i < 64; ++i)
	{
		if (newBoard)
			board.append(cleanValue);
		else
			board[i] = cleanValue;
	}
}

ChessConnector::ChessConnector(QObject* parent)
	: QObject(parent),
	_game(GameAptr(new Game()))
{
	_game->RegisterBoardChanged(
		[&](int index, const Piece& piece)
		{
			emit boardChanged(index, QString::fromStdString(piece.ToString()));
		});

	_game->RegisterGameActionsListeners(
		[&](const EventBase& event)
		{
			if (event.GetType() == EtCheck)
			{
				emit checkNotify();
			}
			else if (event.GetType() == EtCheckMate)
			{
				_gameOver = true;
				emit checkMateNotify();
			}
			else if (event.GetType() == EtCastling)
			{
				emit castlingNotify();
			}
			else if (event.GetType() == EtPawnPromotion)
			{
				const PawnPromotionEvent& ppEvent = (PawnPromotionEvent&)event;
				_onPawnPromotedCallback = ppEvent.OnPromoted;
				emit pawnPromotionNotify(ppEvent.GetIndex(), ppEvent.GetColor());
			}
		});

	_game->RegisterLogger(
		[&](const std::string& message)
		{
			qDebug() << "[Game] " << message.c_str();
		});

	ClearBoard(_possibleMoves);
}

int ChessConnector::MoveCount()
{
	return _game->GetMoveCount();
}

int ChessConnector::IsWhiteMove()
{
	return _game->IsWhiteMove() ? 1 : 0;
}

void ChessConnector::figureSelected(int index)
{
	if (IsOnPlayerMode()) {
		return;
	}

	if (_engineThinking) {
		return;
	}

	//find possible moves for the position and notify IU
	auto pmString = _possibleMoves[index];

	auto selected = pmString == EmptyFlag
		? -1
		: pmString.toInt();

	ClearBoard(_possibleMoves);
	if (selected != -1)
	{
		makeMove(BoardPosition(selected), BoardPosition(index));

		if (_engineWorker && !_gameOver)
		{
			_engineThinking = true;
			emit requestEngineMove();
		}
	}
	else
	{
		for (auto move : _game->GetAllowedMoves(index))
		{
			_possibleMoves[move.To] = QString::number(index);
		}
	}

	emit PossibleMovesChanged();
}

void ChessConnector::pawnPromote(int index, const QString& piece)
{
	auto promotedPiece = Piece::Parse(piece.toStdString());
	_onPawnPromotedCallback({ (BoardPosition)index, promotedPiece });

	qDebug() << "Pawn promoted at " << index << " to "
		<< piece;
}

QStringList& ChessConnector::PossibleMoves()
{
	return _possibleMoves;
}

void ChessConnector::setPossibleMoves(const QStringList& moves)
{
	_possibleMoves = moves;
	emit PossibleMovesChanged();
}

void ChessConnector::EmitMoveCountUpdates()
{
	emit MoveCountChanged();
	emit IsWhiteMoveChanged();
}

void ChessConnector::makeMove(int from, int to)
{
	try
	{
		_game->DoMove((BoardPosition)from, (BoardPosition)to);
		EmitMoveCountUpdates();
	}
	catch (ChessException& ex)
	{
		qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
	}
}

void ChessConnector::startNewGame()
{
	deleteAutoSave();
	stopEngineThread();
	_player = nullptr;
	_gameOver = false;
	_game->Restart();
	EmitMoveCountUpdates();
	emit canContinueChanged();

	qDebug() << "Cpp Game restarted!";
}

void ChessConnector::startNewGameWithComputer()
{
	deleteAutoSave();
	stopEngineThread();
	_player = nullptr;
	_game->EndGame();
	_gameOver = false;
	EmitMoveCountUpdates();
	emit canContinueChanged();
	startEngineThread();
}

void ChessConnector::startEngineThread()
{
	_engineWorker = new EngineWorker(_game);
	_engineThread = new QThread(this);
	_engineWorker->moveToThread(_engineThread);

	connect(this, &ChessConnector::requestEngineMove, _engineWorker, &EngineWorker::doMove);
	connect(_engineWorker, &EngineWorker::moveComplete, this, &ChessConnector::onEngineMoveComplete);
	connect(_engineWorker, &EngineWorker::moveError, this, &ChessConnector::onEngineMoveError);

	// Destroy the worker on the engine thread when it finishes.
	// Qt processes QEvent::DeferredDelete after emitting finished(), so QProcess
	// is destroyed on its owning thread — no cross-thread QObject event assertion.
	connect(_engineThread, &QThread::finished, _engineWorker, &QObject::deleteLater);

	_engineThread->start();
}

void ChessConnector::stopEngineThread()
{
	if (!_engineThread)
		return;

	disconnect(this, &ChessConnector::requestEngineMove, _engineWorker, &EngineWorker::doMove);

	// Kill the stockfish process so any blocking waitForReadyRead() in the engine
	// thread returns immediately, allowing the thread to exit cleanly.
	// QProcess::kill() → TerminateProcess() is an OS-level call, safe cross-thread.
	_engineWorker->killEngine();

	_engineThread->quit();
	_engineThread->wait(3000);

	// _engineWorker was deleted on the engine thread via the finished->deleteLater
	// connection above. Just null the pointer here.
	_engineWorker = nullptr;

	delete _engineThread;
	_engineThread = nullptr;

	_engineThinking = false;
}

void ChessConnector::onEngineMoveComplete()
{
	_engineThinking = false;
	EmitMoveCountUpdates();
}

void ChessConnector::onEngineMoveError(const QString& message)
{
	_engineThinking = false;
	qDebug() << "[Engine] move error:" << message;
}

QString pathAppend(const QString& path1, const QString& path2)
{
	return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString getSaveGameFilePath()
{
	auto path = pathAppend(QDir::currentPath(), DefaultSaveGameFile);
	return path;
}

bool fileExists(QString path) {
	QFileInfo checkFile(path);

	if (checkFile.exists() && checkFile.isFile()) {
		return true;
	}
	else {
		return false;
	}
}

void ChessConnector::saveGame()
{
	_game->Save(getSaveGameFilePath().toStdString());
	emit savedOk();
	emit canLoadChanged();
}

bool ChessConnector::loadGame()
{
	auto success = false;
	try {
		if (fileExists(getSaveGameFilePath()))
		{
			_game->Load(getSaveGameFilePath().toStdString());
			_player = _game->MakePlayer();
			emit IsOnPlayerModeChanged();
			success = true;
		}
	}
	catch (ChessException& ex)
	{
		qDebug() << "Exception caught while loading game " << ex.what();
	}

	if (!success)
	{
		emit noSavedGame();
	}

	return success;
}

void ChessConnector::moveNext()
{
	if (!_player->CanMove(true)) {
		emit noMoreMovesNotify();
	}

	_player->MoveNext();
	EmitMoveCountUpdates();
}

void ChessConnector::movePrev()
{

	if (!_player->CanMove(false)) {
		emit noMoreMovesNotify();
	}

	_player->MoveBack();
	EmitMoveCountUpdates();
}

int ChessConnector::IsOnPlayerMode()
{
	return _player ? 1 : 0;
}

void ChessConnector::endGame()
{
	if (_game->GetMoveCount() > 0 && !IsOnPlayerMode())
		autoSaveGame(_engineWorker != nullptr);

	stopEngineThread();
	_player = nullptr;
	_game->EndGame();
	_gameOver = false;
	EmitMoveCountUpdates();
	emit IsOnPlayerModeChanged();
	ClearBoard(_possibleMoves);
	emit PossibleMovesChanged();
	emit canContinueChanged();
}

bool ChessConnector::continueGame()
{
	bool isSingle = readAutoSaveIsSinglePlayer();

	stopEngineThread();
	_game->Load(getAutoSaveFilePath().toStdString());
	_game->ResumeFromLoad();
	_gameOver = false;

	deleteAutoSave();
	emit canContinueChanged();
	EmitMoveCountUpdates();

	if (isSingle) {
		startEngineThread();
		if (!_game->IsWhiteMove()) {
			_engineThinking = true;
			emit requestEngineMove();
		}
	}
	return isSingle;
}

bool ChessConnector::canContinue() const
{
	return fileExists(getAutoSaveFilePath());
}

bool ChessConnector::canLoad() const
{
	return fileExists(getSaveGameFilePath());
}

QString ChessConnector::getAutoSaveFilePath() const
{
	return pathAppend(QDir::currentPath(), "chess.autosave");
}

QString ChessConnector::getAutoSaveModePath() const
{
	return pathAppend(QDir::currentPath(), "chess.autosave.mode");
}

void ChessConnector::autoSaveGame(bool isSinglePlayer)
{
	_game->Save(getAutoSaveFilePath().toStdString());
	QFile modeFile(getAutoSaveModePath());
	if (modeFile.open(QIODevice::WriteOnly | QIODevice::Text))
		modeFile.write(isSinglePlayer ? "1" : "0");
}

void ChessConnector::deleteAutoSave()
{
	QFile::remove(getAutoSaveFilePath());
	QFile::remove(getAutoSaveModePath());
}

bool ChessConnector::readAutoSaveIsSinglePlayer() const
{
	QFile f(getAutoSaveModePath());
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	return f.readAll().trimmed() == "1";
}

ChessConnector::~ChessConnector()
{
	stopEngineThread();
	qDebug() << "Game Exited.";
}

