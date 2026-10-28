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

// GameResult of a checkmate; the move history marks the last move with '#' only for these.
static const QString MateResultWhite = QStringLiteral("1-0 White victorious");
static const QString MateResultBlack = QStringLiteral("0-1 Black victorious");

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
				flagKing(_game->IsWhiteMove()); // the side to move is mated
				// Once per game: undo after mate resumes the game, and a second mate must
				// not count again.
				if (_userManager && !_resultRecorded) {
					_resultRecorded = true;
					bool humanWon = _game->IsWhiteMove() != _config.playerPlaysWhite;
					_userManager->recordResult(humanWon, _engineWorker != nullptr);
				}
				_gameResult = _game->IsWhiteMove() ? MateResultBlack : MateResultWhite;
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
				_promotionPending = true;
				emit pawnPromotionNotify(ppEvent.GetIndex(), ppEvent.GetColor());
			}
		});

	_game->RegisterLogger(
		[&](const std::string& message)
		{
			qDebug() << "[Game] " << message.c_str();
		});

	ClearBoard(_possibleMoves);

	connect(this, &ChessConnector::reviewStateChanged, this, &ChessConnector::flaggedKingChanged);
}

bool ChessConnector::isMateBy(bool white) const
{
	return _gameResult == (white ? MateResultWhite : MateResultBlack);
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
	if (IsOnPlayerMode() || reviewMode() || _engineThinking || _promotionPending) {
		return;
	}

	// No moves once the game has ended (mate, resignation, Lichess result). Undo against the
	// robot clears _gameOver to resume.
	if (_gameOver)
		return;

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
	if (!_promotionPending)
		return; // e.g. a second click on the promotion dialog

	_promotionPending = false;
	auto promotedPiece = Piece::Parse(piece.toStdString());
	_onPawnPromotedCallback({ (BoardPosition)index, promotedPiece });

	qDebug() << "Pawn promoted at " << index << " to "
		<< piece;

	// The move is only complete now; the history shows the chosen piece (c8=Q).
	buildFullHistoryCache();
	onHumanMoveDone();
}

// Everything that needs the final move: counters, the online opponent, the engine's reply.
void ChessConnector::onHumanMoveDone()
{
	EmitMoveCountUpdates();

	if (!_onlineGameId.isEmpty() && _lichessClient) {
		const auto& rec = _game->GetGameRecord();
		if (!rec.empty())
			_lichessClient->postMove(_onlineGameId,
				QString::fromStdString(rec.back().ToUciString()));
	}

	if (_engineWorker && !_gameOver && _engineAutoPlay)
	{
		_engineThinking = true;
		emit engineThinkingChanged();
		emit requestEngineMove();
	}
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
	bool isMate = isMateBy(i % 2 == 0);
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
		bool wMate = (i == (int)history.size() - 1) && isMateBy(true);
		row["w"] = toFan(Chess::FormatMoveSan(history[i], replayGame->GetCurrentBoard(), wMate),
			history[i].From.Piece.Color);
		if (i + 1 < (int)history.size()) {
			replayGame->DoMove(history[i + 1].ToMove());
			bool bMate = (i + 1 == (int)history.size() - 1) && isMateBy(false);
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
	if (!result.isEmpty()) {
		_gameOver = true;
		ClearBoard(_possibleMoves); // drop move highlights of a selected piece
		emit PossibleMovesChanged();
	}
	emit gameResultChanged();
}

void ChessConnector::flagKing(bool white)
{
	const auto color = white ? Chess::PieceColors::Light : Chess::PieceColors::Dark;
	for (int i = 0; i < 64; ++i) {
		const auto piece = _game->GetPieceAt(i);
		if (piece.Type == Chess::KING && piece.Color == color) {
			setFlaggedKing(i);
			return;
		}
	}
}

void ChessConnector::setFlaggedKing(int square)
{
	if (_flaggedKing == square) return;
	_flaggedKing = square;
	emit flaggedKingChanged();
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

		// A pawn reaching the last rank waits for the player's piece choice; the rest
		// of the move handling runs from pawnPromote() so nobody sees "c7c8" without a piece.
		if (_promotionPending)
			return;

		appendMoveToHistory();
		onHumanMoveDone();
	}
	catch (ChessException& ex)
	{
		qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
	}
}

void ChessConnector::startOnlineGame(const QString& gameId, bool playingAsWhite)
{
	stopEngineThread();
	_promotionPending = false;
	_player = nullptr;
	_gameOver = false;
	if (!_gameResult.isEmpty()) { _gameResult = ""; emit gameResultChanged(); }
	_engineAutoPlay = false;
	_onlineGameId = gameId;
	emit isOnlineGameChanged();
	_wasVsComputer = false;
	beginGame(true);

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
	if (_gameOver)
		return;
	// The robot's move would still be played after the resignation: resign once it lands.
	if (_engineThinking) {
		_resignAfterEngineMove = true;
		return;
	}
	// vs Robot the human resigns. Two players on one device: the side to move resigns.
	const bool whiteResigns = _wasVsComputer ? _config.playerPlaysWhite : IsWhiteMove() == 1;
	_gameOver = true;
	flagKing(whiteResigns);
	_gameResult = whiteResigns ? "0-1 Black won by resignation" : "1-0 White won by resignation";
	emit gameResultChanged();
	emit checkMateResult(whiteResigns ? QStringLiteral("Black Won") : QStringLiteral("White Won"));
}

void ChessConnector::offerDraw()
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->offerDraw(_onlineGameId, true);
}

void ChessConnector::claimVictory()
{
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->claimVictory(_onlineGameId);
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

void ChessConnector::setLichessClient(LichessClient* lc)
{
	_lichessClient = lc;
	if (_lichessClient)
		connect(_lichessClient, &LichessClient::canRematchChanged,
			this, &ChessConnector::canRematchChanged, Qt::UniqueConnection);
	emit canRematchChanged();
}

// Called whenever a new game starts (local, vs Robot, continued, or online).
void ChessConnector::beginGame(bool online)
{
	setFlaggedKing(-1);
	_resultRecorded = false;
	_resignAfterEngineMove = false;
	if (_lastGameOnline == online) return;
	_lastGameOnline = online;
	emit canRematchChanged();
}

void ChessConnector::rematch()
{
	// Lichess has no rematch endpoint: send a new challenge with the same settings.
	// The game starts through LichessClient::gameStarted → startOnlineGame().
	if (_lastGameOnline) {
		if (_lichessClient)
			_lichessClient->createRematch();
		return;
	}
	if (_wasVsComputer)
		startNewGameWithComputer(_config.lastLevel);
	else
		startNewGame();
}

void ChessConnector::startNewGame()
{
	_wasVsComputer = false;
	beginGame(false);
	resetMoveHistoryCache();
	deleteAutoSave();
	stopEngineThread();
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
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
	_wasVsComputer = true;
	beginGame(false);
	resetMoveHistoryCache();
	if (_config.lastLevel != level) {
		_config.lastLevel = level;
		emit lastLevelChanged();
	}
	_config.save();
	deleteAutoSave();
	stopEngineThread();
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
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

QVariantList ChessConnector::levels() const
{
	static const QVariantList list = [] {
		QVariantList res;
		for (int i = 1; i <= Chess::EngineLevel::Count; ++i) {
			Chess::EngineLevel level(i);
			res.append(QVariantMap{
				{ "level",        i },
				{ "name",         QString::fromUtf8(level.Name()) },
				{ "elo",          level.DisplayElo() },
				{ "eloEstimated", !level.UsesEngineElo() },
				{ "hint",         QString::fromUtf8(level.Hint()) },
			});
		}
		return res;
	}();
	return list;
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
	if (_resignAfterEngineMove) {
		_resignAfterEngineMove = false;
		resignGame(); // no-op if the robot's move ended the game
	}
}

void ChessConnector::onEngineMoveError(const QString& message, const QString& details)
{
	_engineThinking = false;
	emit engineThinkingChanged();
	qDebug() << "[Engine] move error:" << message;

	// One log line per dump line so the board and the move list stay readable in the log file.
	for (const auto& line : details.split('\n', Qt::SkipEmptyParts))
		qDebug().noquote() << "[Engine]" << line;

	if (_resignAfterEngineMove) {
		_resignAfterEngineMove = false;
		resignGame();
	}
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
			_promotionPending = false;
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
			if (_engineThinking || _promotionPending) return;
			int count = _game->GetMoveCount();
			if (count == 0) { emit noMoreMovesNotify(); return; }
			// Take back until it is the player's turn: normally the robot's reply and the
			// player's move; after the player mated the robot, only the player's move.
			const bool robotToMove = _game->IsWhiteMove() != _config.playerPlaysWhite;
			int movesToUndo = (robotToMove || count < 2) ? 1 : 2;
			for (int i = 0; i < movesToUndo; i++)
				_game->UndoMove();
			// Taking back the last moves of a finished game (mate or resign) resumes it.
			if (_gameOver) {
				_gameOver = false;
				setFlaggedKing(-1);
				if (!_gameResult.isEmpty()) { _gameResult = ""; emit gameResultChanged(); }
			}
			buildFullHistoryCache();
			ClearBoard(_possibleMoves);
			emit PossibleMovesChanged();
			EmitMoveCountUpdates();
			// E.g. the robot's own first move was taken back: let it play again.
			if (_game->IsWhiteMove() != _config.playerPlaysWhite) {
				_engineThinking = true;
				emit engineThinkingChanged();
				emit requestEngineMove();
			}
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
	setFlaggedKing(-1);
	_resignAfterEngineMove = false;
	if (!_onlineGameId.isEmpty()) {
		if (_lichessClient) _lichessClient->stopStream();
		_onlineGameId.clear();
		emit isOnlineGameChanged();
	}
	else if (_game->GetMoveCount() > 0 && !_gameOver && !IsOnPlayerMode()) {
		// A finished game (mate / resignation) is not offered for Continue.
		autoSaveGame(_engineWorker != nullptr);
	}

	stopEngineThread();
	_promotionPending = false;
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
	_wasVsComputer = isSingle;
	beginGame(false);

	_config.playerPlaysWhite = info.playerPlaysWhite;
	emit playerPlaysWhiteChanged();

	if (_avatarProvider)
		_avatarProvider->restore(info.playerAvatarName, info.opponentAvatarName, isSingle);

	stopEngineThread();
	_promotionPending = false;
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
		if (_game->IsWhiteMove() != _config.playerPlaysWhite && !_promotionPending) {
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

