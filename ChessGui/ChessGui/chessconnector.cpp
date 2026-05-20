#include "stdafx.h"
#include "chessconnector.h"
#include "ChessException.h"
#include "AppConfig.h"
#include "San.h"

#include <QDir>
#include <QDebug>
#include <QFile>
#include <QThread>

using namespace Chess;

const char* EmptyFlag = " ";

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
	_game(GameAptr(new Game())),
	_config(AppConfig::load())
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
				emit checkMateNotify();
				if (_player) return;
				_gameOver = true;
				if (_userManager) {
					bool humanWon = _game->IsWhiteMove() != _config.playerPlaysWhite;
					_userManager->recordResult(humanWon, _engineWorker != nullptr);
				}
				_gameResult = _game->IsWhiteMove() ? "0-1 Black victorious" : "1-0 White victorious";
				emit gameResultChanged();
				QString winner = _game->IsWhiteMove() ? "Black Won" : "White Won";
				emit checkMateResult(winner);
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
	if (IsOnPlayerMode() || reviewMode() || _engineThinking) {
		return;
	}

	// In online games only allow moving when it's the local player's turn.
	if (!_onlineGameId.isEmpty()) {
		const bool localIsWhite = _config.playerPlaysWhite;
		if (_game->IsWhiteMove() != localIsWhite)
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

		if (_engineWorker && !_gameOver && _engineAutoPlay)
		{
			_engineThinking = true;
			emit engineThinkingChanged();
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

namespace {
	static const char pieceCodes[] = " nbrqkp";
	QChar pieceCode(const Chess::Piece& p) { return QChar(pieceCodes[p.Type]); }

	static const char32_t whiteFigurines[] = { 0, 0x2658, 0x2657, 0x2656, 0x2655, 0x2654 };
	static const char32_t blackFigurines[] = { 0, 0x265E, 0x265D, 0x265C, 0x265B, 0x265A };

	static QString pieceSymbol(Chess::PieceTypes type, Chess::PieceColors color) {
		if (type < 1 || type > 5) return {};
		auto cp = (color == Chess::PieceColors::Light) ? whiteFigurines[type] : blackFigurines[type];
		return QString::fromUcs4(&cp, 1);
	}

	static std::shared_ptr<Chess::Game> replayHistory(const Chess::MovesHistory& history, int count) {
		auto game = std::make_shared<Chess::Game>();
		for (int i = 0; i < count && i < (int)history.size(); ++i)
			game->DoMove(history[i].ToMove());
		return game;
	}

	// Replaces the leading ASCII piece letter (N/B/R/Q/K) with the UTF-8 figurine for the given color.
	static QString toFan(const std::string& san, Chess::PieceColors color) {
		if (san.empty())
			return {};
		static const char* const letters = "NBRQK"; // indexed: N=1 B=2 R=3 Q=4 K=5
		const char* p = std::strchr(letters, san[0]);
		if (!p)
			return QString::fromStdString(san);
		auto type = static_cast<Chess::PieceTypes>(p - letters + 1);
		return pieceSymbol(type, color) + QString::fromUtf8(san.c_str() + 1);
	}
}

void ChessConnector::EmitMoveCountUpdates(bool emitHistoryChanged)
{
	emit moveCountChanged();
	emit IsWhiteMoveChanged();
	emit lastMoveChanged();
	emit capturedChanged();
	if (emitHistoryChanged)
		emit moveHistoryChanged();
	emit reviewStateChanged();
}

void ChessConnector::resetMoveHistoryCache() {
	_moveHistoryCache.clear();
}

void ChessConnector::appendMoveToHistory() {
	const auto& rec = _game->GetGameRecord();
	if (rec.empty()) return;
	int i = (int)rec.size() - 1;
	bool isMate = (i % 2 == 0) ? _gameResult.contains("1-0")
		: _gameResult.contains("0-1");
	const auto& m = rec[i];
	QString san = toFan(Chess::FormatMoveSan(m, _game->GetCurrentBoard(), isMate),
		m.From.Piece.Color);
	if (i % 2 == 0) {
		QVariantMap row;
		row["n"] = i / 2 + 1;
		row["w"] = san;
		row["b"] = QString();
		_moveHistoryCache.append(row);
	}
	else if (!_moveHistoryCache.isEmpty()) {
		auto row = _moveHistoryCache.last().toMap();
		row["b"] = san;
		_moveHistoryCache[_moveHistoryCache.size() - 1] = row;
	}
}

void ChessConnector::buildFullHistoryCache() {
	const auto& history = _player ? _player->GetHistory() : _game->GetGameRecord();
	_moveHistoryCache.clear();
	auto replayGame = replayHistory(history, 0);
	for (int i = 0; i < (int)history.size(); i += 2) {
		QVariantMap row;
		row["n"] = i / 2 + 1;
		replayGame->DoMove(history[i].ToMove());
		bool wMate = (i == (int)history.size() - 1) && _gameResult.contains("1-0");
		row["w"] = toFan(Chess::FormatMoveSan(history[i], replayGame->GetCurrentBoard(), wMate),
			history[i].From.Piece.Color);
		if (i + 1 < (int)history.size()) {
			replayGame->DoMove(history[i + 1].ToMove());
			bool bMate = (i + 1 == (int)history.size() - 1) && _gameResult.contains("0-1");
			row["b"] = toFan(Chess::FormatMoveSan(history[i + 1], replayGame->GetCurrentBoard(), bMate),
				history[i + 1].From.Piece.Color);
		}
		else {
			row["b"] = QString();
		}
		_moveHistoryCache.append(row);
	}
}

QVariantList ChessConnector::moveHistory() const
{
	return _moveHistoryCache;
}

void ChessConnector::setGameResult(const QString& result)
{
	_gameResult = result;
	emit gameResultChanged();
}

int ChessConnector::lastMoveFrom() const
{
	const auto& rec = _game->GetGameRecord();
	if (reviewMode())
		return (_reviewIndex == 0 || rec.empty()) ? -1 : (int)rec[_reviewIndex - 1].From.Position;
	return rec.empty() ? -1 : (int)rec.back().From.Position;
}

int ChessConnector::lastMoveTo() const
{
	const auto& rec = _game->GetGameRecord();
	if (reviewMode())
		return (_reviewIndex == 0 || rec.empty()) ? -1 : (int)rec[_reviewIndex - 1].To.Position;
	return rec.empty() ? -1 : (int)rec.back().To.Position;
}

QStringList ChessConnector::capturedByDark() const
{
	QStringList result;
	for (const auto& m : _game->GetGameRecord())
		if (m.IsCapturingMove() && m.To.Piece.Color == Chess::PieceColors::Light)
			result << QString(pieceCode(m.To.Piece).toUpper()); // uppercase = white piece image key
	return result;
}

QStringList ChessConnector::capturedByLight() const
{
	QStringList result;
	for (const auto& m : _game->GetGameRecord())
		if (m.IsCapturingMove() && m.To.Piece.Color == Chess::PieceColors::Dark)
			result << QString(pieceCode(m.To.Piece).toLower()); // lowercase = black piece image key
	return result;
}

void ChessConnector::makeMove(int from, int to)
{
	try
	{
		_game->DoMove((BoardPosition)from, (BoardPosition)to);
		appendMoveToHistory();
		EmitMoveCountUpdates();

		if (!_onlineGameId.isEmpty() && _lichessClient) {
			const auto& rec = _game->GetGameRecord();
			if (!rec.empty())
				_lichessClient->postMove(_onlineGameId,
					QString::fromStdString(rec.back().ToUciString()));
		}
	}
	catch (ChessException& ex)
	{
		qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
	}
}

void ChessConnector::startOnlineGame(const QString& gameId, bool playingAsWhite)
{
	stopEngineThread();
	_player = nullptr;
	_gameOver = false;
	if (!_gameResult.isEmpty()) { _gameResult = ""; emit gameResultChanged(); }
	_engineAutoPlay = false;
	_onlineGameId = gameId;
	emit isOnlineGameChanged();

	_config.playerPlaysWhite = playingAsWhite;
	emit playerPlaysWhiteChanged();

	_game->Restart();
	EmitMoveCountUpdates();
	emit newGameStarted(false);

	if (_lichessClient) {
		connect(_lichessClient, &LichessClient::opponentMoveReceived,
			this, &ChessConnector::applyMoves,
			Qt::UniqueConnection);
		connect(_lichessClient, &LichessClient::drawOfferReceived,
			this, &ChessConnector::drawOfferReceived,
			Qt::UniqueConnection);
		connect(_lichessClient, &LichessClient::takebackRequested,
			this, &ChessConnector::takebackRequested,
			Qt::UniqueConnection);
	}
}

void ChessConnector::resignGame()
{
	if (!_onlineGameId.isEmpty() && _lichessClient) {
		_lichessClient->resign(_onlineGameId);
		return;
	}
	// Local game: white to move resigns → Black wins, and vice versa
	_gameOver = true;
	const QString winner = (IsWhiteMove() == 1) ? QStringLiteral("Black Won")
	                                              : QStringLiteral("White Won");
	emit checkMateResult(winner);
}

void ChessConnector::offerDraw()
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->offerDraw(_onlineGameId, true);
}

void ChessConnector::respondDraw(bool accept)
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->offerDraw(_onlineGameId, accept);
}

void ChessConnector::requestTakeback()
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->requestTakeback(_onlineGameId, true);
	else
		movePrev();
}

void ChessConnector::respondTakeback(bool accept)
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->requestTakeback(_onlineGameId, accept);
}

void ChessConnector::startNewGame()
{
	resetMoveHistoryCache();
	deleteAutoSave();
	stopEngineThread();
	_player = nullptr;
	_gameOver = false;
	if (!_gameResult.isEmpty()) { _gameResult = ""; emit gameResultChanged(); }
	_game->Restart();
	EmitMoveCountUpdates();
	emit canContinueChanged();
	emit newGameStarted(false);

	qDebug() << "Cpp Game restarted!";
}

void ChessConnector::startNewGameWithComputer(int level)
{
	resetMoveHistoryCache();
	_config.lastLevel = level;
	_config.save();
	deleteAutoSave();
	stopEngineThread();
	_player = nullptr;
	_game->EndGame();
	_gameOver = false;
	if (!_gameResult.isEmpty()) { _gameResult = ""; emit gameResultChanged(); }
	EmitMoveCountUpdates();
	emit canContinueChanged();
	emit newGameStarted(true);
	_engineAutoPlay = true;
	startEngineThread(Chess::EngineLevel(level));
	if (!_config.playerPlaysWhite) {
		_engineThinking = true;
		emit engineThinkingChanged();
		emit requestEngineMove();
	}
}

void ChessConnector::applyMoves(const QString& movesStr)
{
	const auto moveList = movesStr.trimmed().split(' ', Qt::SkipEmptyParts);
	for (const auto& moveStr : moveList)
	{
		try
		{
			_game->DoMove(Chess::Move::Parse(moveStr.toStdString()));
		}
		catch (const std::exception& ex)
		{
			qDebug() << "applyMoves: invalid move" << moveStr << ":" << ex.what();
			break;
		}
	}
	buildFullHistoryCache();
	EmitMoveCountUpdates();
}

void ChessConnector::setUseFen(bool v)
{
	if (_config.useFen == v) return;
	_config.useFen = v;
	_config.save();
	emit useFenChanged();
}

void ChessConnector::setPlayerPlaysWhite(bool v)
{
	if (_config.playerPlaysWhite == v) return;
	_config.playerPlaysWhite = v;
	_config.save();
	emit playerPlaysWhiteChanged();
}

void ChessConnector::robotMove()
{
	if (!_engineThread)
		startEngineThread(Chess::EngineLevel{ _config.lastLevel });
	_engineThinking = true;
	emit engineThinkingChanged();
	emit requestEngineMove();
}

void ChessConnector::startEngineThread(Chess::EngineLevel level)
{
	auto mode = _config.useFen
		? Chess::PositionMode::FenWindow
		: Chess::PositionMode::StartPos;
	_engineWorker = new EngineWorker(_game, level, mode);
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
	_engineAutoPlay = false;
	emit engineThinkingChanged();
}

void ChessConnector::onEngineMoveComplete()
{
	_engineThinking = false;
	emit engineThinkingChanged();
	if (reviewMode()) {
		_reviewIndex = -1;
		emitBoardState((int)_game->GetGameRecord().size());
	}
	appendMoveToHistory();
	EmitMoveCountUpdates();
}

void ChessConnector::onEngineMoveError(const QString& message)
{
	_engineThinking = false;
	emit engineThinkingChanged();
	qDebug() << "[Engine] move error:" << message;
}

QString ChessConnector::autoSavePath() const
{
	return AppConfig::autoSaveFilePath(_userManager ? _userManager->activeUserId() : QString());
}

QString ChessConnector::savedGamePath() const
{
	return AppConfig::savedGameFilePath(_userManager ? _userManager->activeUserId() : QString());
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
	if (_userManager) {
		UserManager::GameSaveInfo info;
		info.isSinglePlayer = _engineWorker != nullptr;
		info.playerPlaysWhite = _config.playerPlaysWhite;
		if (_avatarProvider) {
			info.playerAvatarName = _avatarProvider->playerRawName();
			info.opponentAvatarName = _avatarProvider->opponentRawName();
		}
		_userManager->setSavedGameInfo(info);
	}
	_game->Save(savedGamePath().toStdString());
	emit savedOk();
	emit canLoadChanged();
}

bool ChessConnector::loadGame()
{
	auto success = false;
	try {
		if (fileExists(savedGamePath()))
		{
			const auto info = _userManager ? _userManager->savedGameInfo() : UserManager::GameSaveInfo{};
			_config.playerPlaysWhite = info.playerPlaysWhite;
			emit playerPlaysWhiteChanged();
			if (_avatarProvider)
				_avatarProvider->restore(info.playerAvatarName, info.opponentAvatarName, info.isSinglePlayer);
			_game->Load(savedGamePath().toStdString());
			_game->Restart();
			_player = _game->MakePlayer();
			buildFullHistoryCache();
			emit IsOnPlayerModeChanged();
			EmitMoveCountUpdates();
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
	if (!_player) { reviewNext(); return; }

	if (!_player->CanMove(true)) {
		emit noMoreMovesNotify();
		return;
	}
	_player->MoveNext();
	EmitMoveCountUpdates(false);
}

void ChessConnector::movePrev()
{
	if (!_player) {
		if (_engineWorker && !reviewMode()) {
			if (_engineThinking) return;
			int count = _game->GetMoveCount();
			if (count == 0) { emit noMoreMovesNotify(); return; }
			int movesToUndo = count >= 2 ? 2 : 1;
			for (int i = 0; i < movesToUndo; i++)
				_game->UndoMove();
			if (!_moveHistoryCache.isEmpty())
				_moveHistoryCache.removeLast();
			ClearBoard(_possibleMoves);
			emit PossibleMovesChanged();
			EmitMoveCountUpdates();
			return;
		}
		reviewPrev();
		return;
	}

	if (!_player->CanMove(false)) {
		emit noMoreMovesNotify();
		return;
	}
	_player->MoveBack();
	EmitMoveCountUpdates(false);
}

bool ChessConnector::canReviewPrev() const {
	if (_player) return _player->CanMove(false);
	int cur = reviewMode() ? _reviewIndex : (int)_game->GetGameRecord().size();
	return cur > 0;
}

bool ChessConnector::canReviewNext() const {
	if (_player) return _player->CanMove(true);
	return reviewMode() && _reviewIndex < (int)_game->GetGameRecord().size();
}

void ChessConnector::emitBoardState(int moveIndex) {
	const auto& rec = _player ? _player->GetHistory() : _game->GetGameRecord();
	auto replayGame = replayHistory(rec, moveIndex);
	const auto& board = replayGame->GetCurrentBoard();
	for (int i = 0; i < 64; ++i)
		emit boardChanged(i, QString::fromStdString(board.At(Chess::BoardPosition(i)).ToString()));
	ClearBoard(_possibleMoves);
	emit PossibleMovesChanged();
	emit lastMoveChanged();
}

void ChessConnector::reviewFirst() {
	if (_player) {
		while (_player->CanMove(false)) _player->MoveBack();
		EmitMoveCountUpdates(false);
		return;
	}
	_reviewIndex = 0;
	emitBoardState(0);
	emit reviewStateChanged();
}

void ChessConnector::reviewPrev() {
	if (_player) {
		if (!_player->CanMove(false)) { emit noMoreMovesNotify(); return; }
		_player->MoveBack();
		EmitMoveCountUpdates(false);
		return;
	}
	int cur = reviewMode() ? _reviewIndex : (int)_game->GetGameRecord().size();
	if (cur <= 0) return;
	_reviewIndex = cur - 1;
	emitBoardState(_reviewIndex);
	emit reviewStateChanged();
}

void ChessConnector::reviewNext() {
	if (_player) {
		if (!_player->CanMove(true)) { emit noMoreMovesNotify(); return; }
		_player->MoveNext();
		EmitMoveCountUpdates(false);
		return;
	}
	if (!reviewMode()) return;
	if (_reviewIndex >= (int)_game->GetGameRecord().size() - 1) { reviewLast(); return; }
	_reviewIndex++;
	emitBoardState(_reviewIndex);
	emit reviewStateChanged();
}

void ChessConnector::reviewLast() {
	if (_player) {
		while (_player->CanMove(true)) _player->MoveNext();
		EmitMoveCountUpdates();
		return;
	}
	if (!reviewMode()) return;
	_reviewIndex = -1;
	emitBoardState((int)_game->GetGameRecord().size());
	emit reviewStateChanged();
}

void ChessConnector::loginWithLichess()
{
	if (_lichessAuth) return; // already in progress

	_lichessAuth = new LichessAuth(this);

	connect(_lichessAuth, &LichessAuth::loginSucceeded,
		this, [this](const QString& token, const QString& username) {
		if (_userManager)
			_userManager->saveLichessCredentials(token, username);
		if (_lichessClient)
			_lichessClient->setToken(token);
		emit lichessLoginResult(true, username);
		_lichessAuth->deleteLater();
		_lichessAuth = nullptr;
	});

	connect(_lichessAuth, &LichessAuth::loginFailed,
		this, [this](const QString& error) {
		qDebug() << "[LichessAuth] login failed:" << error;
		emit lichessLoginResult(false, QString());
		_lichessAuth->deleteLater();
		_lichessAuth = nullptr;
	});

	_lichessAuth->startLogin();
}

int ChessConnector::IsOnPlayerMode()
{
	return _player ? 1 : 0;
}

void ChessConnector::endGame()
{
	resetMoveHistoryCache();
	if (!_onlineGameId.isEmpty()) {
		if (_lichessClient) _lichessClient->stopStream();
		_onlineGameId.clear();
		emit isOnlineGameChanged();
	}
	else if (_game->GetMoveCount() > 0 && !IsOnPlayerMode()) {
		autoSaveGame(_engineWorker != nullptr);
	}

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
	resetMoveHistoryCache();
	const auto info = _userManager ? _userManager->autoSaveInfo() : UserManager::GameSaveInfo{};
	const bool isSingle = info.isSinglePlayer;

	_config.playerPlaysWhite = info.playerPlaysWhite;
	emit playerPlaysWhiteChanged();

	if (_avatarProvider)
		_avatarProvider->restore(info.playerAvatarName, info.opponentAvatarName, isSingle);

	stopEngineThread();
	_game->Load(autoSavePath().toStdString());
	_game->ResumeFromLoad();
	_gameOver = false;

	deleteAutoSave();
	buildFullHistoryCache();
	emit canContinueChanged();
	EmitMoveCountUpdates();

	if (isSingle) {
		_engineAutoPlay = true;
		startEngineThread(Chess::EngineLevel(_config.lastLevel));
		if (_game->IsWhiteMove() != _config.playerPlaysWhite) {
			_engineThinking = true;
			emit engineThinkingChanged();
			emit requestEngineMove();
		}
	}
	return isSingle;
}

bool ChessConnector::canContinue() const
{
	return fileExists(autoSavePath());
}

bool ChessConnector::canLoad() const
{
	return fileExists(savedGamePath());
}

void ChessConnector::autoSaveGame(bool isSinglePlayer)
{
	_game->Save(autoSavePath().toStdString());
	if (_userManager) {
		UserManager::GameSaveInfo info;
		info.isSinglePlayer = isSinglePlayer;
		info.playerPlaysWhite = _config.playerPlaysWhite;
		if (_avatarProvider) {
			info.playerAvatarName = _avatarProvider->playerRawName();
			info.opponentAvatarName = _avatarProvider->opponentRawName();
		}
		_userManager->setAutoSaveInfo(info);
	}
}

void ChessConnector::deleteAutoSave()
{
	QFile::remove(autoSavePath());
	if (_userManager) _userManager->clearAutoSaveInfo();
}

ChessConnector::~ChessConnector()
{
	if (_game->GetMoveCount() > 0 && !_gameOver && !IsOnPlayerMode())
		autoSaveGame(_engineWorker != nullptr);

	stopEngineThread();
	qDebug() << "Game Exited.";
}

