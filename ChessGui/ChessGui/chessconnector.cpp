#include "stdafx.h"
#include "chessconnector.h"
#include "ChessException.h"
#include "AppConfig.h"
#include "San.h"

#include <algorithm>
#include <QClipboard>
#include <QDate>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QThread>

using namespace Chess;

const char* EmptyFlag = " ";

// GameResult of a checkmate.
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
				const bool whiteMated = _game->IsWhiteMove(); // the side to move is mated
				_gameOver = true;
				flagKing(whiteMated);
				// Once per game: undo after mate resumes the game, and a second mate must
				// not count again.
				if (_userManager && !_resultRecorded) {
					_resultRecorded = true;
					bool humanWon = whiteMated != _config.playerPlaysWhite;
					_userManager->recordResult(humanWon, _engineWorker != nullptr);
				}
				setResult(whiteMated ? MateResultBlack : MateResultWhite,
					sideName(!whiteMated) + QStringLiteral(" wins by checkmate"));
				emit checkMateResult(whiteMated ? QStringLiteral("Black Won") : QStringLiteral("White Won"));
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
	connect(this, &ChessConnector::reviewStateChanged, this, &ChessConnector::capturedChanged); // of the reviewed position
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
	// "N" for a white knight, "n" for a black one: the piece image keys QML uses.
	QString pieceKey(Chess::PieceTypes type, Chess::PieceColors color) {
		const QChar code(pieceCodes[type]);
		return color == Chess::PieceColors::Light ? QString(code.toUpper()) : QString(code);
	}

	static std::shared_ptr<Chess::Game> replayHistory(const Chess::MovesHistory& history, int count) {
		auto game = std::make_shared<Chess::Game>();
		for (int i = 0; i < count && i < (int)history.size(); ++i)
			game->DoMove(history[i].ToMove());
		return game;
	}

	// A move for the move list: the piece image key (none for pawns and castling) and the rest.
	void addMoveCell(QVariantMap& row, const char* side, int ply, const std::string& san) {
		static const std::string letters = "NBRQK";
		const bool hasPiece = !san.empty() && letters.find(san[0]) != std::string::npos;
		const QString text = QString::fromStdString(san);
		row[QStringLiteral("%1Ply").arg(side)]   = ply;
		row[QStringLiteral("%1Piece").arg(side)] = hasPiece ? (ply % 2 == 1 ? text.left(1) : text.left(1).toLower()) : QString();
		row[QStringLiteral("%1Text").arg(side)]  = hasPiece ? text.mid(1) : text;
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
	_sanCache.clear();
}

const Chess::MovesHistory& ChessConnector::shownHistory() const
{
	return _player ? _player->GetHistory() : _game->GetGameRecord();
}

// Rebuilt after every move: FormatHistorySan replays the game to find check and mate.
void ChessConnector::buildFullHistoryCache() {
	_sanCache = Chess::FormatHistorySan(shownHistory());
	_moveHistoryCache.clear();
	for (int i = 0; i < (int)_sanCache.size(); i += 2) {
		QVariantMap row;
		row["n"] = i / 2 + 1;
		addMoveCell(row, "w", i + 1, _sanCache[i]);
		if (i + 1 < (int)_sanCache.size())
			addMoveCell(row, "b", i + 2, _sanCache[i + 1]);
		else
			addMoveCell(row, "b", 0, {});
		_moveHistoryCache.append(row);
	}
}

QVariantList ChessConnector::moveHistory() const
{
	return _moveHistoryCache;
}

int ChessConnector::shownPly() const
{
	if (_player) return _player->GetPointer();
	return reviewMode() ? _reviewIndex : (int)_game->GetGameRecord().size();
}

QString ChessConnector::pgn() const
{
	const QString date = QDate::currentDate().toString(QStringLiteral("yyyy.MM.dd"));
	const QString site = isOnlineGame() ? QStringLiteral("https://lichess.org/") + _onlineGameId
	                                    : QStringLiteral("Chess++");
	const QString result = _gameResult.isEmpty() ? QStringLiteral("*") : _gameResult.section(' ', 0, 0);
	auto tag = [](const char* name, const QString& value) {
		return Chess::PgnTag{ name, value.toStdString() };
	};
	const std::vector<Chess::PgnTag> tags = {
		tag("Event",  QStringLiteral("Casual game")),
		tag("Site",   site),
		tag("Date",   date),
		tag("Round",  QStringLiteral("-")),
		tag("White",  sideName(true)),
		tag("Black",  sideName(false)),
		tag("Result", result),
	};
	return QString::fromStdString(Chess::FormatPgn(tags, _sanCache, result.toStdString()));
}

QString ChessConnector::pgnFileName() const
{
	QString name = sideName(true) + QStringLiteral(" vs ") + sideName(false) + QLatin1Char(' ')
		+ QDate::currentDate().toString(Qt::ISODate) + QStringLiteral(".pgn");
	static const QRegularExpression notInFileNames(QStringLiteral("[<>:\"/\\\\|?*]"));
	return name.replace(notInFileNames, QStringLiteral("_"));
}

void ChessConnector::copyPgn()
{
	QGuiApplication::clipboard()->setText(pgn());
	emit pgnCopied();
}

void ChessConnector::exportPgn(const QUrl& file)
{
	const QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
	QSaveFile out(path);
	const bool ok = out.open(QIODevice::WriteOnly | QIODevice::Text)
		&& out.write(pgn().toUtf8()) >= 0
		&& out.commit();
	if (!ok)
		qDebug() << "exportPgn: can't write" << path << out.errorString();
	emit pgnExported(ok, QDir::toNativeSeparators(path));
}

void ChessConnector::setResult(const QString& result, const QString& text)
{
	_gameResult = result;
	_resultText = text;
	if (!result.isEmpty()) {
		_gameOver = true;
		ClearBoard(_possibleMoves); // drop move highlights of a selected piece
		emit PossibleMovesChanged();
		setPendingOffer({});
		setOpponentGone(false);
	}
	emit gameResultChanged();
}

void ChessConnector::setPendingOffer(const QString& offer)
{
	if (_pendingOffer == offer) return;
	_pendingOffer = offer;
	emit pendingOfferChanged();
}

void ChessConnector::setOpponentGone(bool gone, int claimWinInSeconds)
{
	if (!gone) claimWinInSeconds = -1;
	if (_opponentGone == gone && _claimWinSeconds == claimWinInSeconds) return;
	_opponentGone = gone;
	_claimWinSeconds = claimWinInSeconds;
	emit opponentGoneChanged();
}

void ChessConnector::setGameMode(const QString& mode, const QString& opponentName)
{
	_gameMode = mode;
	_opponentName = opponentName;
	emit gameInfoChanged(); // also when only the opponent changed: the title shows it
}

QString ChessConnector::playerName() const
{
	const QString name = _userManager ? _userManager->activeUserName() : QString();
	return name.isEmpty() ? QStringLiteral("You") : name;
}

QString ChessConnector::sideName(bool white) const
{
	return white == _config.playerPlaysWhite ? playerName() : _opponentName;
}

// The source has no /utf-8: non-ASCII characters only as \u escapes in u"" (QStringLiteral) strings.
static const QString Dot = QStringLiteral(" \u00B7 ");

static QString vsRobotTitle(int level)
{
	return QStringLiteral("vs Robot") + Dot + QStringLiteral("Level %1").arg(level);
}

QString ChessConnector::gameTitle() const
{
	if (_gameMode == QLatin1String("computer"))
		return vsRobotTitle(_config.lastLevel);
	if (_gameMode == QLatin1String("online"))
		return QStringLiteral("Lichess") + Dot + QStringLiteral("vs ") + _opponentName;
	if (_gameMode == QLatin1String("replay"))
		return QStringLiteral("Saved game") + Dot + sideName(true) + QStringLiteral(" vs ") + sideName(false);
	if (_gameMode == QLatin1String("local"))
		return sideName(true) + QStringLiteral(" vs ") + sideName(false);
	return {};
}

QString ChessConnector::continueDescription() const
{
	if (!_userManager || !canContinue())
		return {};
	const auto info = _userManager->autoSaveInfo();
	if (info.isSinglePlayer)
		return vsRobotTitle(_config.lastLevel);
	return QStringLiteral("vs ") + (info.opponentName.isEmpty() ? QStringLiteral("Guest") : info.opponentName);
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

// Up to the position on the board (while reviewing, the reviewed one), in value order P N B R Q.
QStringList ChessConnector::capturedPieces(Chess::PieceColors capturedColor) const
{
	static const std::vector<Chess::PieceTypes> order = { Chess::PAWN, Chess::KNIGHT, Chess::BISHOP, Chess::ROOK, Chess::QUEEN };
	const auto& rec = _game->GetGameRecord();
	const int count = reviewMode() ? std::min(_reviewIndex, (int)rec.size()) : (int)rec.size();
	std::vector<int> taken(Chess::EPC_MAX, 0);
	for (int i = 0; i < count; ++i) {
		const auto& m = rec[i];
		if (m.IsCapturingMove() && m.To.Piece.Color == capturedColor)
			++taken[m.To.Piece.Type];
		else if (m.IsEnPassantMove() && m.From.Piece.Color != capturedColor)
			++taken[Chess::PAWN]; // the pawn taken en passant isn't on the target square
	}
	QStringList result;
	for (auto type : order)
		for (int n = 0; n < taken[type]; ++n)
			result << pieceKey(type, capturedColor);
	return result;
}

QStringList ChessConnector::capturedByDark() const
{
	return capturedPieces(Chess::PieceColors::Light);
}

QStringList ChessConnector::capturedByLight() const
{
	return capturedPieces(Chess::PieceColors::Dark);
}

int ChessConnector::materialBalance() const
{
	if (!reviewMode())
		return Chess::MaterialBalance(_game->GetCurrentBoard());
	return Chess::MaterialBalance(replayHistory(_game->GetGameRecord(), _reviewIndex)->GetCurrentBoard());
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

		buildFullHistoryCache();
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
	_reviewIndex = -1;
	_gameOver = false;
	if (!_gameResult.isEmpty()) setResult({}, {});
	setPendingOffer({});
	setOpponentGone(false);
	_engineAutoPlay = false;
	_onlineGameId = gameId;
	emit isOnlineGameChanged();
	_wasVsComputer = false;
	beginGame(true);

	_config.playerPlaysWhite = playingAsWhite;
	emit playerPlaysWhiteChanged();

	resetMoveHistoryCache();
	_game->Restart();
	EmitMoveCountUpdates();
	emit newGameStarted(false);

	if (_lichessClient) {
		connect(_lichessClient, &LichessClient::opponentMoveReceived,
			this, &ChessConnector::applyMoves,
			Qt::UniqueConnection);
	}
}

void ChessConnector::onLichessGameStarted(const QString& gameId, bool playingAsWhite,
	const QString& opponentName, const QString& opponentAvatar)
{
	startOnlineGame(gameId, playingAsWhite);
	if (_avatarProvider)
		_avatarProvider->setOpponentFromUser(opponentAvatar.isEmpty() ? QStringLiteral("unicorn") : opponentAvatar);
	setGameMode(QStringLiteral("online"),
		opponentName.isEmpty() ? QStringLiteral("Anonymous") : opponentName);
}

void ChessConnector::onLichessGameEnded(const QString& status, const QString& winner)
{
	if (_lichessClient)
		_lichessClient->stopStream();

	// Checkmate is already announced by the local game when the mating move arrives.
	if (status == QLatin1String("mate") && !_gameResult.isEmpty())
		return;

	const bool hasWinner = winner == QLatin1String("white") || winner == QLatin1String("black");
	const bool whiteWon  = winner == QLatin1String("white");
	// The side that gave up, ran out of time or left.
	static const QHash<QString, QString> loserText = {
		{ QStringLiteral("resign"),    QStringLiteral(" resigned") },
		{ QStringLiteral("outoftime"), QStringLiteral(" ran out of time") },
		{ QStringLiteral("timeout"),   QStringLiteral(" left the game") },
		{ QStringLiteral("noStart"),   QStringLiteral(" didn't move") },
	};
	const bool loserKnown = hasWinner && loserText.contains(status);

	QString text;
	if (status == QLatin1String("aborted"))
		text = QStringLiteral("Game aborted");
	else if (loserKnown)
		text = sideName(!whiteWon) + loserText.value(status);
	else if (hasWinner)
		text = sideName(whiteWon) + (status == QLatin1String("mate") ? QStringLiteral(" wins by checkmate")
		                                                              : QStringLiteral(" wins"));
	else if (status == QLatin1String("stalemate"))
		text = QStringLiteral("Draw by stalemate");
	else
		text = QStringLiteral("Draw"); // without a winner, e.g. time ran out against a bare king

	// Like Lichess: a red flag on the king of the side that gave up or ran out of time.
	if (loserKnown)
		flagKing(!whiteWon);

	const QString result = status == QLatin1String("aborted") ? QStringLiteral("*")
		: hasWinner ? (whiteWon ? QStringLiteral("1-0") : QStringLiteral("0-1"))
		: QStringLiteral("1/2-1/2");
	setResult(result, text);
}

void ChessConnector::onLichessOpponentGone(bool gone, int claimWinInSeconds)
{
	if (_gameOver && gone)
		return;
	setOpponentGone(gone, claimWinInSeconds);
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
	setResult(whiteResigns ? QStringLiteral("0-1 Black won by resignation") : QStringLiteral("1-0 White won by resignation"),
		sideName(whiteResigns) + QStringLiteral(" resigned"));
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
	setPendingOffer({});
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
	setPendingOffer({});
	if (!_onlineGameId.isEmpty() && _lichessClient)
		_lichessClient->requestTakeback(_onlineGameId, accept);
}

void ChessConnector::setUserManager(UserManager* um)
{
	_userManager = um;
	// Saves are per user: Continue and Open saved game change with the user.
	connect(_userManager, &UserManager::activeUserChanged, this, &ChessConnector::canContinueChanged);
	connect(_userManager, &UserManager::activeUserChanged, this, &ChessConnector::canLoadChanged);
	connect(_userManager, &UserManager::activeUserChanged, this, &ChessConnector::gameInfoChanged); // a renamed profile: the title
}

void ChessConnector::setLichessClient(LichessClient* lc)
{
	_lichessClient = lc;
	if (_lichessClient) {
		connect(_lichessClient, &LichessClient::canRematchChanged,
			this, &ChessConnector::canRematchChanged, Qt::UniqueConnection);
		connect(_lichessClient, &LichessClient::gameStarted, this, &ChessConnector::onLichessGameStarted);
		connect(_lichessClient, &LichessClient::gameEnded,   this, &ChessConnector::onLichessGameEnded);
		connect(_lichessClient, &LichessClient::opponentGone, this, &ChessConnector::onLichessOpponentGone);
		connect(_lichessClient, &LichessClient::drawOfferReceived, this, [this]() {
			if (isOnlineGame() && !_gameOver) setPendingOffer(QStringLiteral("draw"));
		});
		connect(_lichessClient, &LichessClient::takebackRequested, this, [this]() {
			if (isOnlineGame() && !_gameOver) setPendingOffer(QStringLiteral("takeback"));
		});
	}
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

void ChessConnector::startComputerGame(int level, int side)
{
	setPlayerPlaysWhite(side == 2 ? QRandomGenerator::global()->bounded(2) == 0 : side == 0);
	if (_avatarProvider)
		_avatarProvider->randomize(true); // the opponent shows the robot
	startNewGameWithComputer(level);
	setGameMode(QStringLiteral("computer"), QStringLiteral("Robot"));
}

void ChessConnector::startLocalGame(bool playerPlaysWhite, const QString& opponentUserId)
{
	QString name = QStringLiteral("Guest");
	QString avatar;
	if (_userManager) {
		for (const auto& u : _userManager->users()) {
			const auto user = u.toMap();
			if (!opponentUserId.isEmpty() && user.value(QStringLiteral("id")).toString() == opponentUserId) {
				name   = user.value(QStringLiteral("name")).toString();
				avatar = user.value(QStringLiteral("avatarName")).toString();
			}
		}
	}
	setPlayerPlaysWhite(playerPlaysWhite);
	startNewGame();
	if (_avatarProvider) {
		if (avatar.isEmpty()) _avatarProvider->randomize(false); // a Guest gets a random avatar
		else                  _avatarProvider->setOpponentFromUser(avatar);
	}
	setGameMode(QStringLiteral("local"), name);
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
	if (!_gameResult.isEmpty()) setResult({}, {});
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
	if (!_gameResult.isEmpty()) setResult({}, {});
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
				{ "rating",       QStringLiteral("%1%2 Elo").arg(level.UsesEngineElo() ? QString() : QStringLiteral("\u2248")).arg(level.DisplayElo()) },
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
	buildFullHistoryCache();
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
		info.opponentName = _opponentName;
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
			setGameMode(QStringLiteral("replay"), opponentNameOf(info));
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
				if (!_gameResult.isEmpty()) setResult({}, {});
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

void ChessConnector::reviewTo(int ply) {
	if (_player) {
		ply = std::clamp(ply, 0, (int)_player->GetHistory().size());
		while (_player->GetPointer() < ply) _player->MoveNext();
		while (_player->GetPointer() > ply) _player->MoveBack();
		EmitMoveCountUpdates(false);
		return;
	}
	if (ply >= (int)_game->GetGameRecord().size()) {
		reviewLast();
		return;
	}
	_reviewIndex = std::max(0, ply);
	emitBoardState(_reviewIndex);
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
	setPendingOffer({});
	setOpponentGone(false);
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
	_reviewIndex = -1;
	if (!_gameResult.isEmpty()) setResult({}, {});
	EmitMoveCountUpdates();
	emit IsOnPlayerModeChanged();
	ClearBoard(_possibleMoves);
	emit PossibleMovesChanged();
	emit canContinueChanged();
	setGameMode({}, {});
}

void ChessConnector::switchUser()
{
	endGame(); // autosaves the game for the user who played it
	if (_userManager)
		_userManager->logout();
}

void ChessConnector::refreshBoard()
{
	const auto& board = _game->GetCurrentBoard();
	for (int i = 0; i < 64; ++i)
		emit boardChanged(i, QString::fromStdString(board.At(Chess::BoardPosition(i)).ToString()));
}

QString ChessConnector::opponentNameOf(const UserManager::GameSaveInfo& info) const
{
	if (!info.opponentName.isEmpty())
		return info.opponentName;
	return info.isSinglePlayer ? QStringLiteral("Robot") : QStringLiteral("Guest"); // saved before names were kept
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
	setGameMode(isSingle ? QStringLiteral("computer") : QStringLiteral("local"), opponentNameOf(info));
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
		info.opponentName = _opponentName;
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

