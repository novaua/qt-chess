#include "stdafx.h"
#include "chessconnector.h"
#include "frametrace.h"
#include "ChessException.h"
#include "AppConfig.h"
#include "San.h"
#include "gamepgn.h"

#include <algorithm>
#include <utility>
#include <QClipboard>
#include <QDate>
#include <QDeadlineTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScopeGuard>
#include <QThread>

using namespace Chess;

const char* EmptyFlag = " ";

// GameResult of a checkmate.
static const QString MateResultWhite = QStringLiteral("1-0 White victorious");
static const QString MateResultBlack = QStringLiteral("0-1 Black victorious");
static const QString OutOfTimeText = QStringLiteral(" ran out of time");

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
	_pieces = new BoardPieces(this);
	_moveList = new MoveListModel(this);
	_gate = new MoveGate(this);
	connect(_gate, &MoveGate::release, this, [this](const QString& uciMove, MoveGate::Source source) {
		onGateRelease(uciMove, source);
	});
	connect(_gate, &MoveGate::releaseBatch, this, &ChessConnector::playOnlineMoves);
	connect(_gate, &MoveGate::idle, this, &ChessConnector::boardIdle);
	_clock = new GameClock(this);
	_analyzer = new GameAnalyzer(this);
	connect(_analyzer, &GameAnalyzer::runningChanged, this, &ChessConnector::analysisChanged);
	connect(_analyzer, &GameAnalyzer::runningChanged, this, &ChessConnector::analysisProgressChanged);
	connect(_analyzer, &GameAnalyzer::progressChanged, this, &ChessConnector::analysisProgressChanged);
	connect(_analyzer, &GameAnalyzer::finished, this, &ChessConnector::analysisFinished);
	connect(_analyzer, &GameAnalyzer::failed, this, &ChessConnector::analysisFailed);
	connect(_analyzer, &GameAnalyzer::interrupted, this, &ChessConnector::analysisInterrupted);
	// BL-19 step 7: what the board's game's analysis shows follows its moves, its record and the review.
	for (auto changed : { &ChessConnector::moveHistoryChanged, &ChessConnector::gameResultChanged,
	                      &ChessConnector::gameInfoChanged, &ChessConnector::analysisChanged })
		connect(this, changed, this, &ChessConnector::refreshBoardAnalysis);
	connect(this, &ChessConnector::reviewStateChanged, this, &ChessConnector::shownAnalysisChanged);
	// Play from here follows the review and whether the game is over.
	for (auto changed : { &ChessConnector::reviewStateChanged, &ChessConnector::gameResultChanged })
		connect(this, changed, this, &ChessConnector::playFromHereChanged);
	_claimWin = new ClaimWin(this);
	connect(_claimWin, &ClaimWin::goneChanged, this, &ChessConnector::opponentGoneChanged);
	connect(_claimWin, &ClaimWin::secondsLeftChanged, this, &ChessConnector::claimWinSecondsChanged);
	connect(_claimWin, &ClaimWin::canClaimChanged, this, &ChessConnector::canClaimVictoryChanged);
	connect(_clock, &GameClock::flagged, this, &ChessConnector::onClockFlagged);
	_leaveAfterResign.setSingleShot(true);
	connect(&_leaveAfterResign, &QTimer::timeout, this, &ChessConnector::endGame);
	_premoveTimer.setSingleShot(true);
	connect(&_premoveTimer, &QTimer::timeout, this, &ChessConnector::playPremove);
	_pieces->sync(_game->GetCurrentBoard());

	_game->RegisterBoardChanged(
		[&](int, const Piece&)
		{
			schedulePiecesSync();
		});

	_game->RegisterGameActionsListeners(
		[&](const EventBase& event)
		{
			if (event.GetType() == EtCheck)
			{
				// The game page tells about a check in the resumed position once it is shown.
				if (!_resuming)
					emit checkNotify();
			}
			else if (event.GetType() == EtCheckMate)
			{
				if (!_resuming) // a joined game that ended in mate: its page shows the result
					emit checkMateNotify();
				if (_player || _loadingRecord) return;
				const bool whiteMated = _game->IsWhiteMove(); // the side to move is mated
				endGameWithLoser(whiteMated, whiteMated ? MateResultBlack : MateResultWhite,
					sideName(!whiteMated) + QStringLiteral(" wins by checkmate"));
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
	connect(this, &ChessConnector::reviewStateChanged, this, &ChessConnector::premoveChanged); // hidden while reviewing
	connect(this, &ChessConnector::reviewStateChanged, this, &ChessConnector::capturedChanged); // of the reviewed position

	// What canPlayerMove() depends on. (_gameOver changes with the result or a new game.)
	for (auto changed : { &ChessConnector::moveCountChanged, &ChessConnector::engineThinkingChanged,
	                      &ChessConnector::reviewStateChanged, &ChessConnector::IsOnPlayerModeChanged,
	                      &ChessConnector::isOnlineGameChanged, &ChessConnector::playerPlaysWhiteChanged,
	                      &ChessConnector::gameResultChanged })
		connect(this, changed, this, &ChessConnector::scheduleMovableColorChanged);
	connect(this, &ChessConnector::pawnPromotionNotify, this, &ChessConnector::scheduleMovableColorChanged);
	for (auto changed : { &ChessConnector::isOnlineGameChanged, &ChessConnector::gameResultChanged })
		connect(this, changed, this, &ChessConnector::canAcceptChallengeChanged);
}

int ChessConnector::MoveCount()
{
	return _game->GetMoveCount();
}

int ChessConnector::IsWhiteMove()
{
	return _game->IsWhiteMove() ? 1 : 0;
}

bool ChessConnector::canPlayerMove() const
{
	if (_player || reviewMode() || _engineThinking || _promotionPending)
		return false;

	// No moves once the game has ended (mate, resignation, Lichess result). Undo against the
	// robot clears _gameOver to resume.
	if (_gameOver)
		return false;

	// In online games only allow moving when it's the local player's turn.
	if (!_onlineGameId.isEmpty() && _game->IsWhiteMove() != _config.playerPlaysWhite)
		return false;

	// Vs the robot, only the player's own pieces: if its answer never came, the human must not be
	// able to move the robot's pieces and silently swap the sides.
	if (_wasVsComputer && _game->IsWhiteMove() != _config.playerPlaysWhite)
		return false;
	return true;
}

int ChessConnector::movableColor() const
{
	if (canPlayerMove())
		return _game->IsWhiteMove() ? 1 : 2;
	return canPremove() ? (_config.playerPlaysWhite ? 1 : 2) : 0;
}

// Once the current change is complete, as schedulePiecesSync. Always sent, not only when the value
// differs from the last one: some of what movableColor() reads changes without a signal, so an
// "unchanged" could hide a change that a piece made in between has seen.
void ChessConnector::scheduleMovableColorChanged()
{
	if (_movableColorPosted)
		return;
	_movableColorPosted = true;
	QMetaObject::invokeMethod(this, [this] {
		_movableColorPosted = false;
		emit movableColorChanged();
	}, Qt::QueuedConnection);
}

// Not in a two-player game: both sides sit at this board and move in turn.
bool ChessConnector::canPremove() const
{
	if (_player || reviewMode() || _promotionPending || _gameOver)
		return false;
	if (!isOnlineGame() && !_wasVsComputer)
		return false;
	return _game->IsWhiteMove() != _config.playerPlaysWhite;
}

bool ChessConnector::isPlayerPiece(int square) const
{
	const auto own = _config.playerPlaysWhite ? Chess::PieceColors::Light : Chess::PieceColors::Dark;
	return 0 <= square && square < 64 && _game->GetPieceAt(square).Color == own;
}

void ChessConnector::setPremove(int from, int to)
{
	if (_premoveFrom == from && _premoveTo == to)
		return;
	_premoveFrom = from;
	_premoveTo = to;
	emit premoveChanged();
}

void ChessConnector::cancelPremove()
{
	_premoveTimer.stop();
	setPremove(-1, -1);
	if (_premoveDots)
		clearPossibleMoves();
}

// Like Lichess: the premove is played as soon as the opponent's move is there, without waiting for
// its slide (on a clock that time counts, LI-06). Queued after the opponent's move's pieces sync
// (posted to this object before it, so it runs first): its piece starts sliding before ours does.
void ChessConnector::opponentMoveShown()
{
	if (_premoveFrom >= 0)
		QMetaObject::invokeMethod(this, &ChessConnector::playPremove, Qt::QueuedConnection);
	else
		refreshSelection();
}

void ChessConnector::playPremove()
{
	const int from = _premoveFrom;
	const int to = _premoveTo;
	if (from < 0 || reviewMode()) // reviewing: it waits for the live position (reviewLast)
		return;
	if (!canPlayerMove()) {
		if (!canPremove()) // e.g. the opponent's move ended the game
			cancelPremove();
		return;            // not our turn yet: it waits for the next move
	}
	setPremove(-1, -1);
	const int target = moveTarget(from, to); // a Chess960 castling premoved onto the king's square: onto its rook
	if (target < 0) {
		qDebug() << "Premove" << from << "->" << to << "isn't legal any more: dropped";
		refreshSelection(); // dropped silently, as on Lichess
		return;
	}
	clearPossibleMoves();
	// Lichess's premove promotes to a queen: there is no time to ask.
	makeMove(from, target, _game->IsPromotion(from, target) ? Chess::QUEEN : Chess::EMPTY);
}

void ChessConnector::refreshSelection()
{
	const int square = _selectedSquare;
	if (square < 0)
		return;
	clearPossibleMoves();
	figureSelected(square);
}

void ChessConnector::clearPossibleMoves()
{
	// Already clear (a review step, a move without a selection): no signal, so the 64 dots aren't
	// evaluated again for nothing (BL-11).
	if (_selectedSquare < 0 && !_premoveDots
			&& std::all_of(_possibleMoves.cbegin(), _possibleMoves.cend(), [](const QString& m) { return m == EmptyFlag; }))
		return;
	ClearBoard(_possibleMoves);
	_selectedSquare = -1;
	_premoveDots = false;
	FrameTraceScope trace("move dots cleared");
	emit PossibleMovesChanged();
}

// During the opponent's turn the clicks pick a premove instead: our piece shows where it may go
// (a pending premove stays until another one is picked), a dot sets it, any other square cancels it.
// A click on our own piece always selects it, also on a dot: a recapture premove is dragged there.
void ChessConnector::figureSelected(int index)
{
	const bool premove = !canPlayerMove() && canPremove();
	if ((!premove && !canPlayerMove()) || index < 0 || index >= 64)
		return;

	//find possible moves for the position and notify IU
	auto pmString = _possibleMoves[index];
	// The dots and the selection as they were: a click that changes neither (an empty square with
	// nothing selected) doesn't signal (BL-11).
	const QStringList dotsBefore = _possibleMoves;
	const int selectedBefore = _selectedSquare;
	const bool premoveDotsBefore = _premoveDots;

	auto selected = pmString == EmptyFlag
		? -1
		: pmString.toInt();

	ClearBoard(_possibleMoves);
	_selectedSquare = -1;
	_premoveDots = false;
	if (selected != -1 && premove && (!isPlayerPiece(index) || isChess960Castling(selected, index)))
	{
		setPremove(selected, index);
	}
	else if (selected != -1)
	{
		const int target = moveTarget(selected, index);
		makeMove(BoardPosition(selected), BoardPosition(target < 0 ? index : target));
	}
	else if (premove)
	{
		if (isPlayerPiece(index)) {
			for (auto to : _game->GetPremoveTargets(index))
				_possibleMoves[to] = QString::number(index);
			_selectedSquare = index;
			_premoveDots = true;
		}
		else {
			setPremove(-1, -1);
		}
	}
	else
	{
		// A dot wherever a click plays a move (Chess960: also the king's castling square, moveTargets).
		for (const auto& [square, target] : moveTargets(index))
			_possibleMoves[square] = QString::number(index);
		const auto side = _game->IsWhiteMove() ? Chess::PieceColors::Light : Chess::PieceColors::Dark;
		if (_game->GetPieceAt(index).Color == side)
			_selectedSquare = index;
	}

	if (_possibleMoves == dotsBefore && _selectedSquare == selectedBefore && _premoveDots == premoveDotsBefore)
		return;
	FrameTraceScope trace("move dots");
	emit PossibleMovesChanged();
}

bool ChessConnector::movePiece(int from, int to)
{
	// A premove: the piece goes back to its square and the two squares show the move; a drop
	// anywhere else cancels the premove.
	if (!canPlayerMove() && canPremove()) {
		std::vector<Chess::BoardPosition> targets;
		if (isPlayerPiece(from))
			targets = _game->GetPremoveTargets(from);
		const bool target = std::find(targets.begin(), targets.end(), to) != targets.end();
		clearPossibleMoves();
		setPremove(target ? from : -1, target ? to : -1);
		return false;
	}

	const int target = canPlayerMove() ? moveTarget(from, to) : -1;
	clearPossibleMoves();
	if (target >= 0)
		makeMove(from, target);
	return target >= 0;
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
	appendMoveToHistory();
	// Lichess's list changed while the piece was chosen (e.g. a takeback accepted): the move isn't
	// sent, the list replaces it.
	if (_listAfterPromotion) {
		const auto [moves, refused] = *std::exchange(_listAfterPromotion, std::nullopt);
		onOnlineMoveListReplaced(moves, refused);
		return;
	}
	onHumanMoveDone();
}

// Everything that needs the final move: counters, the online opponent, the engine's reply.
void ChessConnector::onHumanMoveDone()
{
	EmitMoveCountUpdates();
	_gate->playerMoveShown(); // the player's move is on the board: the robot's answer waits for it
	moveOnBoard();
	updateRecord();

	if (!_onlineGameId.isEmpty() && _onlineClient) {
		const auto& rec = _game->GetGameRecord();
		if (!rec.empty())
			_onlineClient->postMove(_onlineGameId,
				QString::fromStdString(rec.back().ToUciString()));
	}

	if (_engineWorker && !_gameOver && _engineAutoPlay)
		askEngineMove();
}

// Before the robot is asked: its clock runs while it thinks. A Lichess game: our move switches the
// clock at once, and Lichess's next state brings the exact times.
void ChessConnector::moveOnBoard()
{
	if (_gameOver)
		return;
	if (!isOnlineGame() && _game->IsStalemate()) { // Game only reports a mate
		recordResult(0);
		setResult(QStringLiteral("1/2-1/2 Draw by stalemate"), QStringLiteral("Draw by stalemate"));
		return;
	}
	_clock->moveMade(!_game->IsWhiteMove(), clockPly());
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

// BL-21: worked out from the dots when QML reads it (once per change): a dot's square holds the moving
// piece's square. A capture is a piece of the other side on the target, except straight ahead of a
// pawn (a premove may push onto a piece that should move away first: a pawn never takes there); or,
// for a legal move (not a premove: its pawn may take whatever stands there then), a pawn going aside
// onto an empty square: en passant. A Chess960 king onto its own rook castles: not a capture.
QList<bool> ChessConnector::captureSquares() const
{
	QList<bool> captures(64, false);
	for (int to = 0; to < 64 && to < _possibleMoves.size(); ++to) {
		if (_possibleMoves[to] == EmptyFlag)
			continue;
		bool ok = false;
		const int from = _possibleMoves[to].toInt(&ok);
		if (!ok || from < 0 || from >= 64)
			continue;
		const Piece mover = _game->GetPieceAt(from);
		const Piece target = _game->GetPieceAt(to);
		const bool pawnAside = mover.Type == Chess::PAWN && from % 8 != to % 8;
		if (!target.IsEmpty())
			captures[to] = target.Color != mover.Color && (mover.Type != Chess::PAWN || pawnAside);
		else
			captures[to] = !_premoveDots && pawnAside;
	}
	return captures;
}

namespace {
	static const char pieceCodes[] = " nbrqkp";
	// "N" for a white knight, "n" for a black one: the piece image keys QML uses.
	QString pieceKey(Chess::PieceTypes type, Chess::PieceColors color) {
		const QChar code(pieceCodes[type]);
		return color == Chess::PieceColors::Light ? QString(code.toUpper()) : QString(code);
	}

	// From the game's own start position (a Chess960 one too, LI-01A).
	static std::shared_ptr<Chess::Game> replayHistory(const Chess::MovesHistory& history, int count,
	                                                  const Chess::StartPosition& start) {
		auto game = std::make_shared<Chess::Game>();
		game->Restart(true, start);
		for (int i = 0; i < count && i < (int)history.size(); ++i)
			game->DoMove(history[i].ToMove());
		return game;
	}
}

// The game whose position is on the board: a saved game's replay up to the shown move, a replay
// up to the reviewed move, or the game itself.
std::shared_ptr<Chess::Game> ChessConnector::shownGame() const
{
	if (_player)
		return replayHistory(_player->GetHistory(), _player->GetPointer(), _game->GetStartPosition());
	if (reviewMode())
		return replayHistory(_game->GetGameRecord(), _reviewIndex, _game->GetStartPosition());
	return _game;
}

// A move changes the board square by square (castling: four Place() calls, a takeback of two
// moves: more), so Pieces is compared with the position once, when the change is complete.
void ChessConnector::schedulePiecesSync()
{
	if (_piecesSyncPosted)
		return;
	_piecesSyncPosted = true;
	QMetaObject::invokeMethod(this, [this] {
		_piecesSyncPosted = false;
		FrameTraceScope trace("pieces sync");
		if (_piecesAnew) {
			_piecesAnew = false;
			_pieces->reset(shownGame()->GetCurrentBoard());
		}
		else
			_pieces->sync(shownGame()->GetCurrentBoard());
	}, Qt::QueuedConnection);
}

void ChessConnector::EmitMoveCountUpdates(bool emitHistoryChanged)
{
	FrameTraceScope trace("after-move signals");
	emit moveCountChanged();
	emit IsWhiteMoveChanged();
	updateClaimTurn();
	emit lastMoveChanged();
	if (emitHistoryChanged)
		emit moveHistoryChanged();
	emit reviewStateChanged(); // also capturedChanged (constructor)
	emit canOfferDrawChanged();
}

void ChessConnector::resetMoveHistoryCache() {
	_sanCache.clear();
	_moveList->show(_sanCache);
}

const Chess::MovesHistory& ChessConnector::shownHistory() const
{
	return _player ? _player->GetHistory() : _game->GetGameRecord();
}

// After undo, load or several moves at once: Game::FormatHistorySan replays the game.
void ChessConnector::buildFullHistoryCache() {
	_sanCache = Chess::Game::FormatHistorySan(shownHistory(), _game->GetStartPosition());
	_moveList->show(_sanCache);
}

// After one move of the live game: only that move, with the game's own mate state.
void ChessConnector::appendMoveToHistory() {
	const auto& rec = _game->GetGameRecord();
	if (_player || rec.size() != _sanCache.size() + 1) {
		buildFullHistoryCache();
		return;
	}
	_sanCache.push_back(Chess::FormatMoveSan(rec.back(), _game->GetCurrentBoard(), _game->IsCheckMate()));
	_moveList->show(_sanCache);
}

QString ChessConnector::shownMove() const
{
	const int ply = shownPly();
	if (ply <= 0 || ply > (int)_sanCache.size())
		return {};
	return QStringLiteral("%1%2%3").arg((ply + 1) / 2)
		.arg(ply % 2 == 1 ? QStringLiteral(". ") : QStringLiteral("\u2026 "))
		.arg(QString::fromStdString(_sanCache[ply - 1]));
}

int ChessConnector::shownPly() const
{
	if (_player) return _player->GetPointer();
	return reviewMode() ? _reviewIndex : (int)_game->GetGameRecord().size();
}

// The game on the board as PGN; a kept game shown as a replay is written from its record (the robot's
// level, the clock), the same as from its card.
GamePgn ChessConnector::boardPgn() const
{
	if (_gameMode == QLatin1String("replay") && _library) {
		if (const auto rec = _library->find(_recordId))
			if (auto pgn = GamePgn::ofRecord(*rec, playerName()))
				return *pgn;
	}
	GamePgn pgn(sideName(true), sideName(false), _sanCache);
	pgn.setStartPosition(_game->GetStartPosition());
	pgn.setDate(_gameDate); // unknown ("????.??.??") only for a game begun elsewhere
	pgn.setResult(_gameMode == QLatin1String("replay") ? _replayResult : _gameResult.section(' ', 0, 0));
	if (isOnlineGame())
		pgn.setLichessGame(_onlineGameId);
	else if (_gameMode == QLatin1String("computer")) {
		pgn.setRobot(_config.lastLevel, !_config.playerPlaysWhite);
		if (const auto rec = _library ? _library->find(_recordId) : std::nullopt; rec && rec->rated)
			pgn.setPlayerElo(rec->ratingBefore, _config.playerPlaysWhite);
		pgn.setTimeControl(_clockLimitSec, _clockIncSec);
	}
	else if (_gameMode == QLatin1String("local")) {
		pgn.setTwoPlayers();
		pgn.setTimeControl(_clockLimitSec, _clockIncSec);
	}
	return pgn;
}

// A kept game (gameId) or the game on the board ("").
std::optional<GamePgn> ChessConnector::pgnOf(const QString& gameId) const
{
	if (gameId.isEmpty())
		return boardPgn();
	const auto rec = _library ? _library->find(gameId) : std::nullopt;
	if (!rec)
		return std::nullopt;
	return GamePgn::ofRecord(*rec, playerName());
}

QString ChessConnector::pgn(const QString& gameId) const
{
	const auto pgn = pgnOf(gameId);
	return pgn ? pgn->text() : QString();
}

QString ChessConnector::pgnFileName(const QString& gameId) const
{
	const auto pgn = pgnOf(gameId);
	return pgn ? pgn->fileName() : QStringLiteral("game.pgn");
}

void ChessConnector::copyPgn()
{
	QGuiApplication::clipboard()->setText(pgn());
	emit pgnCopied();
}

void ChessConnector::exportPgn(const QUrl& file, const QString& gameId)
{
	const QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
	const QString text = pgn(gameId);
	QSaveFile out(path);
	const bool ok = file.isLocalFile() && !text.isEmpty()
		&& out.open(QIODevice::WriteOnly | QIODevice::Text)
		&& out.write(text.toUtf8()) >= 0
		&& out.commit();
	if (!ok)
		qDebug() << "exportPgn: can't write" << path << (text.isEmpty() ? QStringLiteral("no such game") : out.errorString());
	emit pgnExported(ok, QDir::toNativeSeparators(path));
}

void ChessConnector::setResult(const QString& result, const QString& text)
{
	_gameResult = result;
	_resultText = text;
	if (!result.isEmpty()) {
		_gameOver = true;
		_clock->pause();      // the times stay on the cards
		clearPossibleMoves(); // drop move highlights of a selected piece
		cancelPremove();
		setPendingOffer({});
		setOpponentGone(false);
	}
	emit gameResultChanged();
	if (!result.isEmpty())
		updateRecord(); // finished: the result and when
}

void ChessConnector::setPendingOffer(const QString& offer)
{
	if (_pendingOffer == offer) return;
	_pendingOffer = offer;
	emit pendingOfferChanged();
}

void ChessConnector::setReconnecting(bool reconnecting)
{
	if (_reconnecting == reconnecting)
		return;
	_reconnecting = reconnecting;
	emit reconnectingChanged();
}

// LI-06: OnlineClient reconnects on its own. Our board keeps the game meanwhile: the player may
// move (the move is posted; the resync tells whether it arrived), and the clock goes on, as on Lichess.
void ChessConnector::onOnlineConnectionChanged(bool connected)
{
	if (!isOnlineGame())
		return;
	setReconnecting(!connected);
	if (connected)
		emit reconnected();
}

void ChessConnector::setOpponentGone(bool gone, int claimWinInSeconds)
{
	updateClaimTurn();
	_claimWin->setGone(gone, claimWinInSeconds);
}

void ChessConnector::updateClaimTurn()
{
	_claimWin->setOpponentToMove(_game->IsWhiteMove() != _config.playerPlaysWhite);
}

void ChessConnector::setGameMode(const QString& mode, const QString& opponentName, const QString& opponentUserId)
{
	_gameMode = mode;
	_opponentName = opponentName;
	_opponentUserId = opponentUserId;
	emit gameInfoChanged(); // also when only the opponent changed: the title shows it
}

// Statistics of every profile that played: vs Robot by level, two players by opponent (both
// profiles get the game; a Guest only counts for the profile), Lichess. Once per game: undo after
// mate resumes the game, and a second mate must not count again.
void ChessConnector::recordResult(int whiteScore)
{
	if (!_userManager || _resultRecorded || _gameMode == QLatin1String("replay") || _gameMode.isEmpty())
		return;
	_resultRecorded = true;
	const QString me = _userManager->activeUserId();
	const int myScore = _config.playerPlaysWhite ? whiteScore : -whiteScore;
	if (_gameMode == QLatin1String("computer")) {
		_userManager->recordResult(me, myScore, UserManager::computerKey(_config.lastLevel), QStringLiteral("Robot"));
	} else if (_gameMode == QLatin1String("online")) {
		_userManager->recordResult(me, myScore, QStringLiteral("lichess"), QStringLiteral("Lichess"));
	} else if (_opponentUserId.isEmpty()) {
		_userManager->recordResult(me, myScore, QStringLiteral("guest"), QStringLiteral("Guest"));
	} else {
		_userManager->recordResult(me, myScore, UserManager::userKey(_opponentUserId), _opponentName);
		_userManager->recordResult(_opponentUserId, -myScore, UserManager::userKey(me), playerName());
	}
}

QString ChessConnector::playerName() const
{
	const QString name = _userManager ? _userManager->activeUserName() : QString();
	return name.isEmpty() ? QStringLiteral("You") : name;
}

QString ChessConnector::sideName(bool white) const
{
	return white == _config.playerPlaysWhite ? shownPlayerName() : _opponentName;
}

// The source has no /utf-8: non-ASCII characters only as \u escapes in u"" (QStringLiteral) strings.
static const QString Dot = QStringLiteral(" \u00B7 ");

// LI-07: a Lichess game shows the Lichess players (the result, the title and the messages name them too).
QString ChessConnector::shownPlayerName() const
{
	if (lichessPlayersShown() && !_lichessMe.name().isEmpty())
		return _lichessMe.name();
	return playerName();
}

QString ChessConnector::playerTitle() const
{
	return lichessPlayersShown() ? _lichessMe.title() : QString();
}

QString ChessConnector::opponentTitle() const
{
	return lichessPlayersShown() ? _lichessOpponent.title() : QString();
}

// The player's profile stays visible beside the Lichess name: "Ninja", Dot, "1523?".
QString ChessConnector::playerDetail() const
{
	if (!lichessPlayersShown())
		return {};
	QStringList parts;
	const QString profile = playerName();
	if (profile.compare(shownPlayerName(), Qt::CaseInsensitive) != 0)
		parts << profile;
	if (const QString rating = _lichessMe.ratingText(); !rating.isEmpty())
		parts << rating;
	return parts.join(Dot);
}

QString ChessConnector::opponentDetail() const
{
	return lichessPlayersShown() ? _lichessOpponent.ratingText() : QString();
}

QString ChessConnector::playerFlair() const
{
	return lichessPlayersShown() ? _lichessMe.flairUrl() : QString();
}

QString ChessConnector::onlineGameUrl() const
{
	const QString id = isOnlineGame() ? _onlineGameId : _keptLichessGameId;
	return id.isEmpty() ? QString() : QStringLiteral("https://lichess.org/") + id;
}

// LI-11: the flairs of the game's players came after its start (OnlineClient asks /api/users): the
// opponent's is their avatar, the player's shows after their name.
void ChessConnector::onOnlinePlayersChanged()
{
	if (!isOnlineGame() || _onlineGameId != _onlineClient->currentGameId())
		return;
	_lichessMe       = _onlineClient->gamePlayer(_config.playerPlaysWhite);
	_lichessOpponent = _onlineClient->gamePlayer(!_config.playerPlaysWhite);
	if (_avatarProvider)
		_avatarProvider->setOpponentOnline(_lichessOpponent.flairUrl());
	emit gameInfoChanged();
}

static QString vsRobotTitle(int level)
{
	return QStringLiteral("vs Robot") + Dot + QStringLiteral("Level %1").arg(level);
}

QString ChessConnector::gameTitle() const
{
	QString title;
	if (_gameMode == QLatin1String("computer"))
		title = vsRobotTitle(_config.lastLevel);
	else if (_gameMode == QLatin1String("online"))
		title = QStringLiteral("Lichess") + Dot + QStringLiteral("vs ")
			+ (opponentTitle().isEmpty() ? QString() : opponentTitle() + QLatin1Char(' ')) + _opponentName;
	else if (_gameMode == QLatin1String("replay"))
		title = (_keptLichessGameId.isEmpty() ? QString() : QStringLiteral("Lichess") + Dot) // LI-14
			+ sideName(true) + QStringLiteral(" vs ") + sideName(false)
			+ (_gameDate.isValid() ? Dot + _gameDate.toString(QStringLiteral("d MMM yyyy")) : QString());
	else if (_gameMode == QLatin1String("local"))
		title = sideName(true) + QStringLiteral(" vs ") + sideName(false);
	else
		return {};
	// LI-01A: a Chess960 game names its position, as lichess.org does ("Chess960 #518" too)
	return chess960() ? title + Dot + QStringLiteral("Chess960 #%1").arg(startPositionNumber()) : title;
}

// BL-17/BL-19: the one unfinished game, local (the library) or on Lichess, starts at once.
bool ChessConnector::continueOnlyGame()
{
	const QList<GameRecord> local = _library ? _library->unfinished() : QList<GameRecord>{};
	const bool linked = _onlineClient && _userManager && _userManager->lichessConnected();
	// Before the Lichess games have ever come, the list shows the local games at once and the Lichess
	// ones when they come. Once they have, a refresh in flight (and there are several in the first
	// seconds after a start) doesn't make the last list wrong: decide on it (BL-24).
	if (linked && !_onlineClient->ongoingGamesLoaded()) {
		qDebug() << "ChessConnector: Continue opens the list | local:" << local.size() << "| Lichess games not loaded yet";
		return false;
	}
	const QVariantList online = linked ? _onlineClient->ongoingGames() : QVariantList{};
	if (local.size() == 1 && online.isEmpty())
		return continueGame(local.first().id);
	if (local.isEmpty() && online.size() == 1 && online.first().toMap().value(QStringLiteral("canPlayHere")).toBool())
		return continueOnlineGame(online.first().toMap().value(QStringLiteral("gameId")).toString());
	qDebug() << "ChessConnector: Continue opens the list | local:" << local.size() << "| Lichess:" << online.size()
	         << (linked && _onlineClient->ongoingGamesLoading() ? "(refreshing)" : "");
	return false;
}

bool ChessConnector::continueOnlineGame(const QString& gameId)
{
	if (gameId.isEmpty() || !_onlineClient || !_userManager || !_userManager->lichessConnected())
		return false;
	_onlineStartUserId = _userManager->activeUserId();
	_startTiming.begin(QStringLiteral("continue on Lichess"));
	_onlineClient->rejoinGame(gameId);
	return true;
}

void ChessConnector::cancelOnlineStart()
{
	_startTiming.cancel();
	if (startingOnlineGame() || !acceptingChallengeId().isEmpty())
		_onlineClient->stopStream(); // no game is on yet: nothing else is lost
}

void ChessConnector::checkOnlineStart()
{
	if ((startingOnlineGame() || !acceptingChallengeId().isEmpty())
		&& (!_userManager->isLoggedIn() || _userManager->activeUserId() != _onlineStartUserId || !_userManager->lichessConnected()))
		cancelOnlineStart();
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
		const auto piece = rec[i].CapturedPiece(); // also en passant
		if (piece.Color == capturedColor)
			++taken[piece.Type];
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

// In the position on the board: live, reviewed, or a saved game's replay.
int ChessConnector::checkedKing() const
{
	const auto shown = shownGame();
	Chess::Game& game = *shown;
	if (!game.IsInCheck())
		return -1;
	const auto color = game.IsWhiteMove() ? Chess::PieceColors::Light : Chess::PieceColors::Dark;
	for (int i = 0; i < 64; ++i) {
		const auto piece = game.GetPieceAt(i);
		if (piece.Type == Chess::KING && piece.Color == color)
			return i;
	}
	return -1;
}

int ChessConnector::materialBalance() const
{
	return Chess::MaterialBalance(shownGame()->GetCurrentBoard());
}

void ChessConnector::makeMove(int from, int to, Chess::PieceTypes promoteTo)
{
	// The player's own move (also in the moment the premove waits for the opponent's slide): a
	// pending premove was meant for the position before it.
	_premoveTimer.stop();
	setPremove(-1, -1);
	try
	{
		_game->DoMove(Move{ (BoardPosition)from, (BoardPosition)to, false, { promoteTo, PieceColors::Empty } });
	}
	catch (ChessException& ex)
	{
		qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
		return;
	}

	// A pawn reaching the last rank waits for the player's piece choice; the rest
	// of the move handling runs from pawnPromote() so nobody sees "c7c8" without a piece.
	if (_promotionPending)
		return;

	appendMoveToHistory();
	onHumanMoveDone();
}

bool ChessConnector::isChess960Castling(int from, int to) const
{
	if (!chess960() || from < 0 || from >= 64 || to < 0 || to >= 64)
		return false;
	const auto king = _game->GetPieceAt(from);
	const auto rook = _game->GetPieceAt(to);
	return king.Type == Chess::KING && rook.Type == Chess::ROOK && rook.Color == king.Color;
}

std::map<int, int> ChessConnector::moveTargets(int from) const
{
	std::map<int, int> targets;
	if (from < 0 || from >= 64)
		return targets;
	const auto moves = _game->GetAllowedMoves(from);
	for (const auto& m : moves)
		targets[m.To] = m.To;
	// Chess960 (as lichess.org): the king's castling square castles too, when no move of its own goes
	// there (emplace keeps that one) and the king leaves its square.
	for (const auto& m : moves)
		if (isChess960Castling(from, m.To)) {
			const int kingTo = Chess::CastlingSquares::Of(BoardPosition(from), m.To).KingTo;
			if (kingTo != from)
				targets.emplace(kingTo, m.To);
		}
	return targets;
}

int ChessConnector::moveTarget(int from, int to) const
{
	const auto targets = moveTargets(from);
	const auto it = targets.find(to);
	return it == targets.end() ? -1 : it->second;
}

void ChessConnector::restartGame(const Chess::StartPosition& start)
{
	const bool changed = !(start == _game->GetStartPosition());
	_game->Restart(true, start);
	if (changed) {
		emit startPositionChanged();
		emit gameInfoChanged(); // the title names a Chess960 position
	}
}

bool ChessConnector::startOnlineGame(const QString& gameId, bool playingAsWhite, const Chess::StartPosition& start,
	const QStringList& joinedMoves)
{
	_clock->stop();
	stopEngineThread();
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
	_gameOver = false;
	if (!_gameResult.isEmpty()) setResult({}, {});
	setPendingOffer({});
	setOpponentGone(false);
	setReconnecting(false);
	_onlineRealTime = false; // until its clock says (onOnlineClockStarted)
	_engineAutoPlay = false;
	_onlineGameId = gameId;
	_drawOfferPly = -1;
	emit isOnlineGameChanged();
	_wasVsComputer = false;
	beginGame(true);

	_config.playerPlaysWhite = playingAsWhite;
	emit playerPlaysWhiteChanged();

	resetMoveHistoryCache();
	restartGame(start); // the joined moves and every move after are played from it
	bool allPlayed = true;
	if (joinedMoves.isEmpty()) {
		EmitMoveCountUpdates();
	} else {
		// BL-18: the position at once, without slides (also on a game page that is already open), sounds
		// or "Check": the page tells a check in the position it opens on (newGameStarted).
		_piecesAnew = true;
		allPlayed = playMoves(joinedMoves, false) == joinedMoves.size();
		// The board is busy for a moment, then idle: the page asks about a check or a result then,
		// also when it was open already.
		_gate->moveShown();
	}
	emit newGameStarted(false);

	if (_onlineClient) {
		connect(_onlineClient, &OnlineClient::opponentMoveReceived,
			this, &ChessConnector::onOnlineOpponentMoves,
			Qt::UniqueConnection);
		connect(_onlineClient, &OnlineClient::moveListReplaced,
			this, &ChessConnector::onOnlineMoveListReplaced,
			Qt::UniqueConnection);
	}
	return allPlayed;
}

void ChessConnector::onOnlineGameStarted(const QString& gameId, bool playingAsWhite,
	const QString& opponentName, const QString& opponentAvatar, const QStringList& moves,
	const QString& variant, const QString& startFen)
{
	_rematchChallengeId.clear(); // a game started: the challenge waits no more (its own game, or another)
	// LI-01A stage 2: standard or Chess960, from the position Lichess gives. Anything else (a link to a
	// Crazyhouse game, a challenge accepted on lichess.org) is refused before it replaces the game on
	// the board: a board out of step with Lichess can't be played move by move.
	std::optional<Chess::StartPosition> start;
	if (variant == QLatin1String("standard") && startFen.isEmpty())
		start = Chess::StartPosition();
	else if (variant == QLatin1String("chess960"))
		start = startFen.isEmpty() ? Chess::StartPosition::Chess960(Chess::StartPosition::StandardNumber) // "startpos": #518
		                           : Chess::StartPosition::FromFen(startFen.toStdString()); // #518 too: castles as Chess960
	if (!start) {
		qWarning() << "ChessConnector: Lichess game" << gameId << "can't be played here | variant:" << variant
		           << "| initialFen:" << startFen;
		_startTiming.cancel();
		emit gameStartFailed(_onlineClient->server().playsVariant(variant)
			? QStringLiteral("its start position isn't one Chess++ can play. Open it on lichess.org.")
			: QStringLiteral("it is %1, and Chess++ plays standard chess and Chess960. "
			                 "Open it on lichess.org.").arg(OnlineClient::variantName(variant)));
		_onlineClient->stopStream(); // the game on the board (if any) stays as it was
		return;
	}
	// A Lichess game replaces a local game (BL-10: a challenge accepted during it): kept for
	// Continue, as when leaving it for Home.
	keepGameOnLeave();
	const QString opponent = opponentName.isEmpty() ? QStringLiteral("Anonymous") : opponentName;
	// BL-18: a game joined in progress (two moves or more: a rejoin) is on the board before setGameMode()
	// opens its page, so the first frame shows its position. Zero or one move: a new game, whose
	// opponent may have moved already: that move comes as the opponent's, through the gate (it slides,
	// with its sound, and plays a premove).
	const bool joined = moves.size() > 1;
	if (!_startTiming.running()) // started on Lichess's side: a challenge accepted there, a rematch
		_startTiming.begin(QStringLiteral("Lichess game"));
	_startTiming.step("Lichess"); // the answer's wait: the network
	// Already this game's mode and opponent while its moves are played: a mate among them is recorded
	// and told as this game's (setGameMode() below announces it).
	_gameMode = QStringLiteral("online");
	_opponentName = opponent;
	_opponentUserId.clear();
	_lichessMe       = _onlineClient->gamePlayer(playingAsWhite);
	_lichessOpponent = _onlineClient->gamePlayer(!playingAsWhite);
	_onlineGameFull  = _onlineClient->currentGameFull(); // LI-14: its record once it ends (a mate among the joined moves too)
	if (!startOnlineGame(gameId, playingAsWhite, *start, joined ? moves : QStringList{})) {
		emit gameStartFailed(QStringLiteral("Lichess sent a move Chess++ can't play"));
		endGame(); // stops the stream; back where the player was (Home), not a board out of step with Lichess
		return;
	}
	// LI-11: a Lichess player: their flair once it comes (onOnlinePlayersChanged), until then (and without
	// one) the standard avatar, never one of the library's.
	Q_UNUSED(opponentAvatar); // Lichess sends none
	if (_avatarProvider)
		_avatarProvider->setOpponentOnline(_lichessOpponent.flairUrl());
	_startTiming.step("setup");
	setGameMode(QStringLiteral("online"), opponent);
	if (!joined && !moves.isEmpty())
		onOnlineOpponentMoves(moves.join(QLatin1Char(' ')));
}

void ChessConnector::onOnlineGameEnded(const QString& status, const QString& winner)
{
	if (!isOnlineGame())
		return; // not the game on the board: its result isn't this game's
	if (_onlineClient) {
		_onlineClient->stopStream();
		_onlineClient->watchForRematch();
	}
	// resignAndLeave(): leave once the result is in, after this signal (not inside it).
	if (_leaveAfterResign.isActive())
		_leaveAfterResign.start(0);

	// The game is over at once (no moves, offers or resignation any more), even if the opponent's
	// last move still waits in the gate: the result card waits for the board (GamePage).
	// Checkmate is already announced by the local game when the mating move arrives.
	if (status == QLatin1String("mate") && !_gameResult.isEmpty())
		return;

	const bool hasWinner = winner == QLatin1String("white") || winner == QLatin1String("black");
	const bool whiteWon  = winner == QLatin1String("white");
	// The side that gave up, ran out of time or left.
	static const QHash<QString, QString> loserText = {
		{ QStringLiteral("resign"),    QStringLiteral(" resigned") },
		{ QStringLiteral("outoftime"), OutOfTimeText },
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
	// Counted as the game is kept (LI-14, keepLichessGame): not aborted, and with a move played, so that
	// statistics rebuilt from the kept games are the same.
	if (status != QLatin1String("aborted") && plyCount() > 0)
		recordResult(hasWinner ? (whiteWon ? 1 : -1) : 0);
	setResult(result, text);
}

void ChessConnector::onOnlineOpponentGone(bool gone, int claimWinInSeconds)
{
	if (_gameOver && gone)
		return;
	setOpponentGone(gone, claimWinInSeconds);
}

void ChessConnector::onOnlineClockStarted(qint64 limitMs, qint64 incrementMs, bool correspondence)
{
	if (!isOnlineGame())
		return; // a Lichess game that isn't on the board (refused): the local game's clock stays its own
	_clock->setLowTimeSides(_config.playerPlaysWhite, !_config.playerPlaysWhite);
	// Real time: the opponent's reply isn't held for our move's slide and pause (up to 0.4 s of our
	// clock on Lichess, LI-06). Days per move: no hurry, the board shows one thing at a time.
	_gate->setHoldOnline(correspondence);
	_onlineRealTime = !correspondence;
	emit canAcceptChallengeChanged();
	if (correspondence)
		_clock->startCorrespondence(limitMs);
	else
		_clock->start(limitMs, incrementMs, false);
}

void ChessConnector::resignGame()
{
	if (!_onlineGameId.isEmpty() && _onlineClient) {
		_onlineClient->resign(_onlineGameId);
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
	endGameWithLoser(whiteResigns, whiteResigns ? QStringLiteral("0-1 Black won by resignation") : QStringLiteral("1-0 White won by resignation"),
		sideName(whiteResigns) + QStringLiteral(" resigned"));
}

void ChessConnector::endGameWithLoser(bool white, const QString& result, const QString& text)
{
	_gameOver = true;
	flagKing(white);
	recordResult(white ? -1 : 1);
	setResult(result, text);
	if (!_resuming)
		emit checkMateResult(white ? QStringLiteral("Black Won") : QStringLiteral("White Won"));
}

// Lichess's rule: the side out of time loses, unless the other side can't mate. Lichess ends its own
// games (GameClock doesn't flag there).
void ChessConnector::onClockFlagged(bool white)
{
	dropRobotAnswer(); // it comes too late
	// The piece was still being chosen: the move isn't finished, so it doesn't stand.
	if (_promotionPending) {
		_promotionPending = false;
		_game->UndoMove();
		emit pawnPromotionCancelled();
	}
	_gameOver = true;
	const QString outOfTime = sideName(white) + OutOfTimeText;
	if (!_game->GetCurrentBoard().CanMate(white ? PieceColors::Dark : PieceColors::Light)) {
		recordResult(0);
		setResult(QStringLiteral("1/2-1/2 Draw on time"), QStringLiteral("Draw: ") + outOfTime);
		return;
	}
	endGameWithLoser(white, white ? QStringLiteral("0-1 Black won on time") : QStringLiteral("1-0 White won on time"), outOfTime);
}

void ChessConnector::dropRobotAnswer()
{
	_gate->clear();
	++_engineRequest;
	_resignAfterEngineMove = false;
	if (_engineThinking) {
		_engineThinking = false;
		emit engineThinkingChanged();
	}
}

bool ChessConnector::canOfferDraw() const
{
	// Lichess's rule (its own button is disabled the same way): not before both sides moved,
	// and not again until 20 plies after our previous offer. Lichess ignores such an offer.
	const int ply = _game->GetMoveCount();
	return isOnlineGame() && ply >= 2 && (_drawOfferPly < 0 || _drawOfferPly < ply - 20);
}

void ChessConnector::offerDraw()
{
	if (!canOfferDraw() || !_onlineClient)
		return;
	_onlineClient->offerDraw(_onlineGameId, true);
	_drawOfferPly = _game->GetMoveCount();
	emit canOfferDrawChanged();
}

void ChessConnector::claimVictory()
{
	if (canClaimVictory() && !_onlineGameId.isEmpty() && _onlineClient)
		_onlineClient->claimVictory(_onlineGameId);
}

void ChessConnector::respondDraw(bool accept)
{
	setPendingOffer({});
	if (!_onlineGameId.isEmpty() && _onlineClient)
		_onlineClient->offerDraw(_onlineGameId, accept);
}

void ChessConnector::requestTakeback()
{
	if (!_onlineGameId.isEmpty() && _onlineClient)
		_onlineClient->requestTakeback(_onlineGameId, true);
	else if (!_clock->timeUp()) // a flag ends the game for good: the side out of time has nothing to go on with
		movePrev();
}

void ChessConnector::respondTakeback(bool accept)
{
	setPendingOffer({});
	if (!_onlineGameId.isEmpty() && _onlineClient)
		_onlineClient->requestTakeback(_onlineGameId, accept);
}

void ChessConnector::respondRematch(bool accept)
{
	setPendingOffer({});
	if (!_onlineClient)
		return;
	if (accept) {
		_startTiming.begin(QStringLiteral("rematch on Lichess"));
		_onlineClient->acceptRematch(); // the game starts through OnlineClient::gameStarted
	}
	else
		_onlineClient->declineRematch();
}

void ChessConnector::setUserManager(UserManager* um)
{
	_userManager = um;
	connect(_userManager, &UserManager::activeUserChanged, this, &ChessConnector::gameInfoChanged); // a renamed profile: the title
	// A rejoin belongs to the profile and the Lichess account that started it.
	connect(_userManager, &UserManager::activeUserChanged, this, &ChessConnector::checkOnlineStart);
	connect(_userManager, &UserManager::lichessChanged, this, &ChessConnector::checkOnlineStart);
}

void ChessConnector::setGameLibrary(GameLibrary* library)
{
	_library = library;
	_analyzer->setLibrary(library);
	if (_library)
		connect(_library, &GameLibrary::gamesChanged, this, &ChessConnector::refreshBoardAnalysis);
}

namespace {
	QString markOf(GameAnalysis::Judgement j)
	{
		switch (j) {
		case GameAnalysis::Judgement::Inaccuracy: return QStringLiteral("?!");
		case GameAnalysis::Judgement::Mistake:    return QStringLiteral("?");
		case GameAnalysis::Judgement::Blunder:    return QStringLiteral("??");
		default:                                  return QString();
		}
	}

	// As lichess.org: "+0.4", "−1.2", "#3" (White mates in 3), "#−2", and the result once mated.
	QString evalText(int cp)
	{
		const QString minus(QChar(0x2212));
		if (GameAnalysis::isMate(cp)) {
			const int n = GameAnalysis::mateIn(cp);
			if (n == 0)
				return cp > 0 ? QStringLiteral("1-0") : QStringLiteral("0-1");
			return n > 0 ? QStringLiteral("#%1").arg(n) : QStringLiteral("#") + minus + QString::number(-n);
		}
		if (cp == 0)
			return QStringLiteral("0.0");
		return (cp > 0 ? QStringLiteral("+") : minus) + QString::number(std::abs(cp) / 100.0, 'f', 1);
	}

	// A UCI move's squares ("e2e4": 12, 28; 0 = a1 … 63 = h8); -1, -1 when it isn't one.
	std::pair<int, int> squaresOf(const QString& uci)
	{
		if (uci.size() < 4)
			return { -1, -1 };
		auto square = [&uci](int at) {
			const int file = uci.at(at).unicode() - 'a';
			const int rank = uci.at(at + 1).unicode() - '1';
			return file >= 0 && file < 8 && rank >= 0 && rank < 8 ? rank * 8 + file : -1;
		};
		const int from = square(0), to = square(2);
		return from < 0 || to < 0 ? std::pair{ -1, -1 } : std::pair{ from, to };
	}
}

// The board's game's analysis, when its record has one for the moves it has now; whether it is being
// analysed (the record of the game on the board changes with a new game, a rematch, a replay: not only
// when an analysis starts or ends) or can be. The move list's marks, the summary and the best moves with it.
void ChessConnector::refreshBoardAnalysis()
{
	GameAnalysis analysis;
	QStringList moves;
	// A Lichess game has a record only once it is over (LI-14): then it is analysed as a local game.
	const bool kept = _library && !_recordId.isEmpty();
	if (kept)
		if (const auto rec = _library->find(_recordId)) {
			analysis = GameAnalysis::fromJson(rec->analysis);
			moves = rec->moves;
			if (!analysis.fits(moves) || analysis.plyCount() != plyCount())
				analysis = {};
		}
	const bool analysing = kept && _analyzer->running() && _analyzer->gameId() == _recordId;
	const bool can = kept && plyCount() > 0 && !analysing
		&& (_gameMode == QLatin1String("replay") || !_gameResult.isEmpty()); // not a game going on: no hints
	const bool changed = !(analysis == _boardAnalysis) || (!analysis.isEmpty() && moves != _analysisMoves);
	if (!changed && can == _canAnalyse && analysing == _analysingThisGame)
		return;
	_boardAnalysis = analysis;
	_analysisMoves = analysis.isEmpty() ? QStringList() : moves;
	_canAnalyse = can;
	_analysingThisGame = analysing;
	if (changed) {
		QStringList marks;
		_bestSan.clear();
		for (int ply = 0; ply < analysis.plyCount(); ++ply) {
			const auto judgement = analysis.judgement(ply);
			marks.append(markOf(judgement));
			const QString best = analysis.bestMove(ply);
			_bestSan.append(judgement != GameAnalysis::Judgement::None && !best.isEmpty() && best != moves.value(ply)
			                ? sanOf(ply, best) : QString());
		}
		_moveList->setMarks(marks);
		_analysisSummary = summaryOf(analysis);
	}
	emit boardAnalysisChanged();
	emit shownAnalysisChanged();
}

QVariantMap ChessConnector::summaryOf(const GameAnalysis& analysis)
{
	if (analysis.isEmpty())
		return {};
	using J = GameAnalysis::Judgement;
	auto side = [&analysis](bool white) {
		const double accuracy = analysis.accuracy(white);
		return QVariantMap{
			{ QStringLiteral("accuracy"),     accuracy < 0 ? -1 : qRound(accuracy) },
			{ QStringLiteral("inaccuracies"), analysis.count(white, J::Inaccuracy) },
			{ QStringLiteral("mistakes"),     analysis.count(white, J::Mistake) },
			{ QStringLiteral("blunders"),     analysis.count(white, J::Blunder) },
		};
	};
	return QVariantMap{
		{ QStringLiteral("depth"), analysis.depth() },
		{ QStringLiteral("white"), side(true) },
		{ QStringLiteral("black"), side(false) },
	};
}

QVariantMap ChessConnector::shownAnalysis() const
{
	if (_boardAnalysis.isEmpty())
		return {};
	const int position = std::clamp(shownPly(), 0, _boardAnalysis.plyCount());
	const int cp = _boardAnalysis.eval(position);
	QVariantMap shown{
		{ QStringLiteral("evalText"),     evalText(cp) },
		{ QStringLiteral("whitePercent"), GameAnalysis::winPercent(cp) },
		{ QStringLiteral("mark"),         QString() },
		{ QStringLiteral("judgement"),    QString() },
		{ QStringLiteral("best"),         QString() },
		{ QStringLiteral("playedFrom"),   -1 },
		{ QStringLiteral("playedTo"),     -1 },
		{ QStringLiteral("bestFrom"),     -1 },
		{ QStringLiteral("bestTo"),       -1 },
	};
	if (position > 0) {
		const auto judgement = _boardAnalysis.judgement(position - 1);
		const QString name = GameAnalysis::judgementName(judgement);
		const QString best = _bestSan.value(position - 1);
		shown[QStringLiteral("mark")] = markOf(judgement);
		shown[QStringLiteral("judgement")] = name.isEmpty() ? name : name.left(1).toUpper() + name.mid(1);
		shown[QStringLiteral("best")] = best;
		// The board's two arrows, with best (a judged move whose engine move was another).
		if (!best.isEmpty()) {
			const auto played = squaresOf(_analysisMoves.value(position - 1));
			const auto engine = squaresOf(_boardAnalysis.bestMove(position - 1));
			if (played.first >= 0 && engine.first >= 0) {
				shown[QStringLiteral("playedFrom")] = played.first;
				shown[QStringLiteral("playedTo")]   = played.second;
				shown[QStringLiteral("bestFrom")]   = engine.first;
				shown[QStringLiteral("bestTo")]     = engine.second;
			}
		}
	}
	return shown;
}

QString ChessConnector::sanOf(int position, const QString& uciMove) const
{
	const auto game = GameRecord::play(_analysisMoves.mid(0, position), _game->GetStartPosition());
	if (!game || !GameRecord::playOn(*game, { uciMove }))
		return {};
	const auto san = Chess::Game::FormatHistorySan(game->GetGameRecord(), game->GetStartPosition());
	return san.empty() ? QString() : QString::fromStdString(san.back());
}

void ChessConnector::setAnalysisDepth(int depth)
{
	const int before = _analyzer->depth();
	_analyzer->setDepth(depth);
	if (_analyzer->depth() != before)
		emit analysisDepthChanged();
}

bool ChessConnector::analyseGame(const QString& gameId)
{
	const QString id = gameId.isEmpty() ? _recordId : gameId;
	if (id.isEmpty())
		return false;
	_analyzer->setPositionMode(_config.useFen ? Chess::PositionMode::FenWindow : Chess::PositionMode::StartPos);
	return _analyzer->start(id);
}

void ChessConnector::cancelAnalysis()
{
	_analyzer->cancel();
}

void ChessConnector::setOnlineClient(OnlineClient* lc)
{
	_onlineClient = lc;
	if (_onlineClient) {
		connect(_onlineClient, &OnlineClient::canRematchChanged,
			this, &ChessConnector::canRematchChanged, Qt::UniqueConnection);
		connect(_onlineClient, &OnlineClient::gameStarted, this, &ChessConnector::onOnlineGameStarted);
		connect(_onlineClient, &OnlineClient::gamePlayersChanged, this, &ChessConnector::onOnlinePlayersChanged);
		connect(_onlineClient, &OnlineClient::rejoiningChanged, this, &ChessConnector::startingOnlineGameChanged);
	connect(_onlineClient, &OnlineClient::acceptingChanged, this, &ChessConnector::acceptingChallengeChanged);
		// A refused Accept, or a Lichess game that never came (a challenge canceled, an error): not timed.
		connect(_onlineClient, &OnlineClient::challengeAcceptFailed, this, [this] { _startTiming.cancel(); });
		connect(_onlineClient, &OnlineClient::challengeCanceled, this, [this] {
			_startTiming.cancel();
			_rematchChallengeId.clear();
		});
		connect(_onlineClient, &OnlineClient::challengeDeclined, this, [this] {
			_startTiming.cancel();
			_rematchChallengeId.clear();
		});
		// BL-36: a challenge made with a game on the board is its rematch (the Online page is reached from
		// Home only); leaving the game withdraws it (endGame()).
		connect(_onlineClient, &OnlineClient::challengeCreated, this, [this](const QString& gameId) {
			if (inGame()) _rematchChallengeId = gameId;
		});
		connect(_onlineClient, &OnlineClient::directChallengeSent, this, [this](const QString& challengeId) {
			if (inGame()) _rematchChallengeId = challengeId;
		});
		// LI-13: a game action Lichess refused, or didn't answer, is said; also after the game was left (a
		// Resign that never got there: the game goes on, and is lost on time). A Resign-and-leave stays.
		connect(_onlineClient, &OnlineClient::gameActionFailed, this,
		        [this](const QString& gameId, const QString& action, const QString& message) {
			const bool thisGame = gameId == _onlineGameId;
			if (thisGame && !_gameResult.isEmpty())
				return; // over anyway (e.g. the opponent's flag at the same time)
			if (thisGame && action == QLatin1String("resign") && _leaveAfterResign.isActive()) {
				_leaveAfterResign.stop();
				emit resignAndLeaveStopped();
			}
			emit gameActionFailed(message);
		});
		// e.g. the game ended meanwhile: the list is out of date.
		connect(_onlineClient, &OnlineClient::rejoinFailed, this, [this](const QString&, const QString& message) {
			_startTiming.cancel();
			emit gameStartFailed(message);
			_onlineClient->refreshOngoingGames();
		});
		connect(_onlineClient, &OnlineClient::gameEnded,   this, &ChessConnector::onOnlineGameEnded);
		connect(_onlineClient, &OnlineClient::opponentGone, this, &ChessConnector::onOnlineOpponentGone);
		connect(_onlineClient, &OnlineClient::clockStarted, this, &ChessConnector::onOnlineClockStarted);
		connect(_onlineClient, &OnlineClient::gameConnectionChanged, this, &ChessConnector::onOnlineConnectionChanged);
		connect(_onlineClient, &OnlineClient::gameStreamStopped, this, [this]() { setReconnecting(false); });
		connect(_onlineClient, &OnlineClient::clockUpdated, this,
			[this](qint64 whiteMs, qint64 blackMs, bool whiteToMove, bool running) {
				if (isOnlineGame()) // Lichess's times are for its game only, never a local game's clock
					_clock->setSnapshot(whiteMs, blackMs, whiteToMove, running);
			});
		// An offer that comes with the opponent's move is shown after the move.
		connect(_onlineClient, &OnlineClient::drawOfferReceived, this, [this]() {
			afterHeldMoves([this] { if (isOnlineGame() && !_gameOver) setPendingOffer(QStringLiteral("draw")); });
		});
		connect(_onlineClient, &OnlineClient::takebackRequested, this, [this]() {
			afterHeldMoves([this] { if (isOnlineGame() && !_gameOver) setPendingOffer(QStringLiteral("takeback")); });
		});
		connect(_onlineClient, &OnlineClient::rematchOffered, this, [this]() {
			if (isOnlineGame() && _gameOver) setPendingOffer(QStringLiteral("rematch"));
		});
		connect(_onlineClient, &OnlineClient::rematchOfferCanceled, this, [this]() {
			if (_pendingOffer == QLatin1String("rematch")) setPendingOffer({});
		});
	}
	emit canRematchChanged();
}

// Called whenever a new game starts (local, vs Robot, continued, or online).
void ChessConnector::beginGame(bool online)
{
	_gate->clear(); // a held move belongs to the game being left
	_gate->setHoldOnline(true);
	cancelPremove();
	_afterHeldMoves.clear();
	_listAfterPromotion.reset();
	_gameDate = QDate::currentDate();
	setFlaggedKing(-1);
	_resultRecorded = false;
	_resignAfterEngineMove = false;
	_recordId.clear(); // a new game: its record is made at its first move (updateRecord)
	_recordStarted = QDateTime::currentDateTime();
	_undoCount = 0;
	_forkOfId.clear();
	_forkPly = 0;
	_replayResult.clear();
	_keptLichessGameId.clear();
	if (_lastGameOnline == online) return;
	_lastGameOnline = online;
	emit canRematchChanged();
}

bool ChessConnector::acceptChallenge(const QString& challengeId)
{
	if (!_onlineClient || !canAcceptChallenge())
		return false;
	_startTiming.begin(QStringLiteral("accept a Lichess challenge"));
	_onlineStartUserId = _userManager ? _userManager->activeUserId() : QString();
	_onlineClient->acceptChallenge(challengeId); // the challenger chose the sides
	return true;
}

void ChessConnector::rematch()
{
	_startTiming.begin(QStringLiteral("rematch"));
	// Lichess has no rematch endpoint: send a new challenge with the same settings.
	// The game starts through OnlineClient::gameStarted → startOnlineGame().
	if (_lastGameOnline) {
		if (_pendingOffer == QLatin1String("rematch"))
			respondRematch(true); // like Lichess: asking back accepts the opponent's offer
		else if (_onlineClient)
			_onlineClient->createRematch();
		return;
	}
	_nextStart = _game->GetStartPosition(); // a Chess960 rematch: the same position (LI-01A)
	// The board turns to the player's new side. Not saved here: a robot game's start saves the config once,
	// and the setup dialogs offer the side chosen in them, not this one (#193).
	if (_rematchSwapsColours)
		applyPlayerSide(!_config.playerPlaysWhite);
	if (_wasVsComputer)
		startNewGameWithComputer(_config.lastLevel);
	else
		startNewGame();
}

void ChessConnector::newOpponent()
{
	if (!isOnlineGame() || _gameResult.isEmpty() || !canSeekAgain() || _onlineClient->seeking())
		return; // a seek waits already: Cancel is the command bar's
	// First: startSeek refuses while a challenge of ours waits. cancelChallenge clears that at once.
	withdrawRematch();
	_onlineClient->seekAgain();
}

// LI-01A: StandardGame, RandomChess960 or a Chess960 position's number.
static Chess::StartPosition startPositionOf(int chess960)
{
	if (0 <= chess960 && chess960 < Chess::StartPosition::Count)
		return Chess::StartPosition::Chess960(chess960);
	return chess960 == ChessConnector::RandomChess960 ? Chess::StartPosition::Random() : Chess::StartPosition();
}

void ChessConnector::startComputerGame(int level, int side, int limitSec, int incSec, int chess960)
{
	_startTiming.begin(QStringLiteral("vs Robot (level %1)").arg(level));
	_nextStart = startPositionOf(chess960);
	_clockLimitSec = limitSec;
	_clockIncSec = incSec;
	setPlayerPlaysWhite(side == 2 ? QRandomGenerator::global()->bounded(2) == 0 : side == 0);
	if (_avatarProvider)
		_avatarProvider->randomize(true); // the opponent shows the robot
	startNewGameWithComputer(level);
	setGameMode(QStringLiteral("computer"), QStringLiteral("Robot"));
}

void ChessConnector::startLocalGame(bool playerPlaysWhite, const QString& opponentUserId, int limitSec, int incSec, int chess960)
{
	_startTiming.begin(QStringLiteral("two players"));
	_nextStart = startPositionOf(chess960);
	_clockLimitSec = limitSec;
	_clockIncSec = incSec;
	// Another profile, or a Guest (no id, or a profile that is gone).
	const QString profileName = _userManager && !opponentUserId.isEmpty() ? _userManager->nameOf(opponentUserId) : QString();
	const QString name = profileName.isEmpty() ? QStringLiteral("Guest") : profileName;
	const QString avatar = profileName.isEmpty() ? QString() : _userManager->avatarOf(opponentUserId);
	const QString opponentId = profileName.isEmpty() ? QString() : opponentUserId;
	setPlayerPlaysWhite(playerPlaysWhite);
	startNewGame();
	if (_avatarProvider) {
		if (avatar.isEmpty()) _avatarProvider->randomize(false); // a Guest gets a random avatar
		else                  _avatarProvider->setOpponentFromUser(avatar);
	}
	setGameMode(QStringLiteral("local"), name, opponentId);
}

void ChessConnector::startNewGame()
{
	_wasVsComputer = false;
	beginGame(false);
	resetMoveHistoryCache();
	stopEngineThread();
	_startTiming.step("stop engine");
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
	_gameOver = false;
	if (!_gameResult.isEmpty()) setResult({}, {});
	restartGame(newGameStart());
	applyStartFrom();
	startClock();
	EmitMoveCountUpdates();
	emit newGameStarted(false);
	_startTiming.step("setup");

	qDebug() << "Cpp Game restarted!";
}

void ChessConnector::startClock()
{
	if (_clockLimitSec == 0 && _clockIncSec == 0) {
		_clock->stop();
		return;
	}
	// The low-time sound is for the people at this computer: both in a two-player game.
	_clock->setLowTimeSides(!_wasVsComputer || _config.playerPlaysWhite, !_wasVsComputer || !_config.playerPlaysWhite);
	_clock->start(_clockLimitSec * 1000LL, _clockIncSec * 1000LL);
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
	_startTiming.step("save");
	stopEngineThread();
	_startTiming.step("stop engine");
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
	restartGame(newGameStart());
	applyStartFrom();
	_gameOver = false;
	if (!_gameResult.isEmpty()) setResult({}, {});
	startClock();
	EmitMoveCountUpdates();
	emit newGameStarted(true);
	_startTiming.step("setup");
	_engineAutoPlay = true;
	startEngineThread(Chess::EngineLevel(level));
	if (_game->IsWhiteMove() != _config.playerPlaysWhite) { // Play from here: the side to move may be either
		askEngineMove();
	}
	_startTiming.step("start engine");
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
	playMoves(movesStr.split(' ', Qt::SkipEmptyParts));
	updateRecord(); // typed-in moves (Alt+S) are the game's too: kept like played ones (BL-19)
}

// Plays the moves on the game, up to the first one that can't be played, then updates the move
// list and the counters once. Only a check in the final position is announced. Returns how many
// were played.
int ChessConnector::playMoves(const QStringList& uciMoves, bool announce)
{
	int played = 0;
	for (const auto& move : uciMoves) {
		_resuming = !announce || played < uciMoves.size() - 1;
		try {
			_game->DoMove(Chess::Move::Parse(move.toStdString()));
		}
		catch (const std::exception& ex) {
			qWarning() << "Move" << move << "can't be played:" << ex.what();
			break;
		}
		++played;
	}
	_resuming = false;
	appendMoveToHistory(); // rebuilds the whole list when several moves came at once
	EmitMoveCountUpdates();
	return played;
}

void ChessConnector::setScreenshotScene(const QVariantMap& scene)
{
	_screenshotScene = scene;
	_clock->setClock([] { return qint64(0); }); // time stands still: the same scene gives the same picture
	_claimWin->freeze();
}

void ChessConnector::setScreenshotClock(int whiteMs, int blackMs)
{
	// While a promotion waits the pawn has moved, but its side's clock still runs.
	_clock->setSnapshot(whiteMs, blackMs, _game->IsWhiteMove() != _promotionPending, true);
}

void ChessConnector::setUseFen(bool v)
{
	if (_config.useFen == v) return;
	_config.useFen = v;
	_config.save();
	emit useFenChanged();
}

void ChessConnector::setRematchSwapsColours(bool on)
{
	if (_rematchSwapsColours == on) return;
	_rematchSwapsColours = on;
	emit rematchSwapsColoursChanged();
}

void ChessConnector::setPlayerPlaysWhite(bool v)
{
	if (_config.playerPlaysWhite == v) return;
	applyPlayerSide(v);
	_config.save();
}

void ChessConnector::applyPlayerSide(bool white)
{
	if (_config.playerPlaysWhite == white) return;
	_config.playerPlaysWhite = white;
	emit playerPlaysWhiteChanged();
	updateClaimTurn();
}

void ChessConnector::startEngineThread(Chess::EngineLevel level)
{
	auto mode = _config.useFen
		? Chess::PositionMode::FenWindow
		: Chess::PositionMode::StartPos;
	ENGINE_TRACE("start thread: level" << level.Level() << (_config.useFen ? "FenWindow" : "StartPos"));
	_engineWorker = new EngineWorker(level, mode, _game->GetStartPosition());
	_engineThread = new QThread(this);
	_engineWorker->moveToThread(_engineThread);

	connect(_engineWorker, &EngineWorker::moveFound, this, &ChessConnector::onEngineMoveFound);
	connect(_engineWorker, &EngineWorker::moveError, this, &ChessConnector::onEngineMoveError);

	// Destroy the worker on the engine thread when it finishes.
	// Qt processes QEvent::DeferredDelete after emitting finished(), so QProcess
	// is destroyed on its owning thread — no cross-thread QObject event assertion.
	connect(_engineThread, &QThread::finished, _engineWorker, &QObject::deleteLater);

	_engineThread->start();
}

void ChessConnector::stopEngineThread()
{
	_gate->clear(); // an answer waiting for the board belongs to the game being left
	if (!_engineThread)
		return;

	++_engineRequest; // an answer still on its way belongs to the old game: drop it
	ENGINE_TRACE("stop thread: was thinking" << _engineThinking << ", next answer must be" << _engineRequest + 1);

	// The old engine's answers no longer reach this game (the request number drops one already queued).
	disconnect(_engineWorker, nullptr, this, nullptr);

	// Don't wait for the thread (BL-16): inside the engine's UCI start it can't be stopped, and the
	// window froze up to a second here (BL-11). It finishes on its own: the worker is deleted on it
	// (finished → deleteLater), then the thread here. Never QThread::terminate() (TerminateThread):
	// the thread can die inside the heap or the log lock and leave it held (BL-15).
	QThread* thread = _engineThread;
	QElapsedTimer stopping;
	stopping.start();
	connect(thread, &QThread::finished, this, [stopping] {
		if (stopping.elapsed() > 20)
			qCInfo(chessTiming).noquote() << QStringLiteral("[Timing] engine stop: the old engine finished %1 ms later (not waited for)").arg(stopping.elapsed());
	});
	connect(thread, &QThread::finished, thread, &QObject::deleteLater); // before quit(): it may finish at once

	// Stop the search: the process is terminated through its own handle (BL-15); a request that
	// has not started yet gives up. Never touch QProcess here: it belongs to the engine thread.
	_engineWorker->killEngine();
	thread->quit();
	_engineThread = nullptr;
	_engineWorker = nullptr;

	_engineThinking = false;
	_engineAutoPlay = false;
	emit engineThinkingChanged();
}

// The app's exit: a running QThread must not be destroyed (it aborts the process). The stopped
// engines' threads are this connector's children until they finish (finished → deleteLater); a stop
// ends even an engine's start at once (BL-16 review), so they are done in a few ms. Up to 3 s for
// all of them; one still running then is left behind (detached), as before.
void ChessConnector::waitForStoppedEngines()
{
	const QDeadlineTimer deadline(3000);
	for (QThread* thread : findChildren<QThread*>(Qt::FindDirectChildrenOnly)) {
		if (thread->wait(deadline))
			continue;
		qWarning() << "Engine thread did not stop within 3 s; leaving it behind";
		thread->setParent(nullptr); // the connector must not delete it
	}
}

// The robot's turn. The engine thread gets a copy of the moves and only looks for a move;
// onEngineMoveFound() plays it here, on the GUI thread, which owns _game (BL-09).
void ChessConnector::askEngineMove()
{
	_engineThinking = true;
	emit engineThinkingChanged();
	auto worker = _engineWorker;
	const int request = ++_engineRequest;
	const auto history = _game->GetGameRecord();
	const int maxMoveMs = int(_clock->moveBudgetMs(!_config.playerPlaysWhite));
	ENGINE_TRACE("ask: request" << request << "," << history.size() << "plies, worker" << (const void*)worker);
	const bool posted = QMetaObject::invokeMethod(worker,
		[worker, request, history, maxMoveMs] { worker->findMove(request, history, maxMoveMs); }, Qt::QueuedConnection);
	ENGINE_TRACE("ask: request" << request << (posted ? "posted" : "NOT posted"));
	Q_UNUSED(posted);
}

void ChessConnector::onEngineMoveFound(int request, const QString& move)
{
	if (request != _engineRequest) {
		ENGINE_TRACE("answer" << move << "to request" << request << "dropped: waiting for" << _engineRequest
			<< ", thinking" << _engineThinking);
		return;
	}
	// The player is looking at an earlier move: the gate doesn't show moves while reviewing, but the
	// robot's clock is running. As with a Lichess opponent: back to the live position, then the move.
	if (reviewMode() && _clock->enabled())
		reviewLast();
	ENGINE_TRACE("answer" << move << "to request" << request << "held until the board is idle");
	_gate->push(move, MoveGate::Source::Robot); // playRobotMove() plays it when the board is idle
}

// The gate released the robot's answer: play it on the GUI thread, which owns _game (BL-09).
// EngineThinking stays true until here, so the player cannot move while the answer waits.
void ChessConnector::playRobotMove(const QString& move)
{
	ENGINE_TRACE("answer" << move << "playing");
	try {
		_game->DoMove(Chess::Move::Parse(move.toStdString()));
	}
	catch (const std::exception& ex) {
		cancelPremove(); // no move came to answer
		onEngineMoveError(_engineRequest, QStringLiteral("the engine's move %1 can't be played: %2").arg(move, ex.what()),
			QStringLiteral("fen: ") + QString::fromStdString(_game->MakeFen()));
		return;
	}
	_engineThinking = false;
	emit engineThinkingChanged();
	ENGINE_TRACE("answer" << move << "played, the player's turn");
	if (reviewMode()) {
		_reviewIndex = -1;
		emitBoardState();
	}
	appendMoveToHistory();
	EmitMoveCountUpdates();
	moveOnBoard();
	updateRecord();
	_gate->moveShown(true);
	if (_resignAfterEngineMove) {
		_resignAfterEngineMove = false;
		resignGame(); // no-op if the robot's move ended the game
	}
	opponentMoveShown();
}

void ChessConnector::onGateRelease(const QString& uciMove, MoveGate::Source source)
{
	if (source == MoveGate::Source::Robot)
		playRobotMove(uciMove);
	else
		playOnlineMoves({ uciMove });
}

// From the Lichess stream: the opponent's move, or all moves of the game when it is (re)joined.
// Our own moves are not echoed (OnlineClient skips them). They wait in the gate like the robot's
// answer; Lichess's own turn rule keeps the player from moving meanwhile (canPlayerMove()).
void ChessConnector::onOnlineOpponentMoves(const QString& uciMoves)
{
	if (!isOnlineGame())
		return;
	// The player is looking at an earlier move: the gate doesn't show moves while reviewing, but
	// the player's clock is running on Lichess. Back to the live position, then the move.
	if (reviewMode())
		reviewLast();
	_gate->push(uciMoves.split(' ', Qt::SkipEmptyParts), MoveGate::Source::Online);
}

// From the Lichess stream: the game's moves as Lichess has them, when they aren't ours plus new
// moves: an accepted takeback (shorter), our move that Lichess refused, or a disagreement. The game
// goes back to the last move both have, then plays the rest of Lichess's list.
void ChessConnector::onOnlineMoveListReplaced(const QString& uciMoves, bool moveRefused)
{
	if (!isOnlineGame())
		return;
	if (_gameOver) { // e.g. our mating move refused because the clock had run out: the result stands
		qWarning() << "Lichess move list replaced after the game ended; not applied:" << uciMoves;
		return;
	}
	if (_promotionPending) { // the piece is being chosen: applied once it is (pawnPromote)
		_listAfterPromotion = { uciMoves, moveRefused };
		return;
	}
	const QStringList target = uciMoves.split(' ', Qt::SkipEmptyParts);
	// A held opponent move may be one of those taken back; the rest of the list is played below.
	// What came with it (a draw offer) still stands on Lichess: it is shown after the list.
	_gate->clear();
	cancelPremove(); // it was for another position
	const auto after = std::move(_afterHeldMoves);
	_afterHeldMoves.clear();
	if (reviewMode())
		reviewLast();
	if (_pendingOffer == QLatin1String("takeback"))
		setPendingOffer({});

	QStringList ours;
	for (const auto& move : _game->GetGameRecord())
		ours << QString::fromStdString(move.ToUciString());
	int common = 0;
	while (common < ours.size() && common < target.size() && ours[common] == target[common])
		++common;
	for (int i = int(ours.size()); i > common; --i) {
		if (moveRefused) // the increment moveMade() gave it; other lists come with Lichess's exact times
			_clock->moveTakenBack(!_game->IsWhiteMove(), clockPly());
		_game->UndoMove();
	}
	buildFullHistoryCache(); // before playMoves(), which only appends to it
	clearPossibleMoves();

	const QStringList rest = target.mid(common);
	if (ours.size() - common + rest.size() > 2)
		_piecesAnew = true; // more than a takeback of two: show the position anew, without slides
	if (!rest.isEmpty())
		playMoves(rest);
	else
		EmitMoveCountUpdates();
	if (moveRefused) // no state follows
		_clock->setTurn(_game->IsWhiteMove(), clockPly());
	_gate->moveShown(); // the pieces slide back: the next move waits for it

	// A refused move has its own message (OnlineClient's networkError).
	if (!moveRefused && rest.isEmpty() && ours.size() > target.size())
		emit movesTakenBack(int(ours.size() - target.size()));
	for (const auto& action : after)
		action();
}

// The gate released the opponent's move, or a resync (more than two at once: shown anew, one sound).
void ChessConnector::playOnlineMoves(const QStringList& uciMoves)
{
	if (playMoves(uciMoves) > 1)
		_piecesAnew = true; // before the posted Pieces sync runs
	// LI-14: a move the gate held past the game's end (a correspondence game, a review) completes the kept
	// game's record (nothing is written while the game goes on).
	updateRecord();
	_gate->moveShown(true); // not the player's move
	if (_gate->holding())
		return;
	opponentMoveShown();
	// What came with the moves (a draw offer, a takeback request) is shown now.
	const auto after = std::move(_afterHeldMoves);
	_afterHeldMoves.clear();
	for (const auto& action : after)
		action();
}

void ChessConnector::afterHeldMoves(std::function<void()> action)
{
	if (_gate->holding())
		_afterHeldMoves.push_back(std::move(action));
	else
		action();
}

// Entering the review of a game on a clock: the moves still held are played now (the board shows an
// earlier position anyway), so the move list is complete and nothing waits for the review to end
// while a clock runs. A Lichess game always has one, on Lichess.
void ChessConnector::showHeldMoves()
{
	if ((isOnlineGame() || _clock->enabled()) && !reviewMode())
		_gate->releaseAll();
}

void ChessConnector::setBoardSettled(bool settled)
{
	ENGINE_TRACE("board" << (settled ? "settled" : "not settled") << ", gate holding" << _gate->holding());
	_gate->setStageReady(settled);
}

// Continue: the clock waits for the page. Not for the board to settle: a review doesn't stop it.
void ChessConnector::gameShown(QObject* window, bool pageCreated)
{
	_startTiming.shown(qobject_cast<QQuickWindow*>(window), pageCreated);
}

// As GameStartTiming::shown: both signals come from the render thread, queued here in the order they
// were sent; a frame whose synchronization came before this call (drawn without the notice) is skipped.
void ChessConnector::waitForStartFrame(QObject* windowObject)
{
	const int wait = ++_startFrameWait;
	struct Connections { QMetaObject::Connection sync, swap; bool synced = false; };
	auto state = std::make_shared<Connections>();
	auto done = [this, wait, state]() {
		QObject::disconnect(state->sync);
		QObject::disconnect(state->swap);
		if (wait != _startFrameWait)
			return; // a newer wait, or this one is over
		++_startFrameWait;
		_startTiming.step("notice");
		emit startFrameShown();
	};
	auto* window = qobject_cast<QQuickWindow*>(windowObject);
	if (window) {
		state->sync = connect(window, &QQuickWindow::afterSynchronizing, this, [state]() { state->synced = true; }, Qt::QueuedConnection);
		state->swap = connect(window, &QQuickWindow::frameSwapped, this, [state, done]() { if (state->synced) done(); }, Qt::QueuedConnection);
		window->update();
	}
	QTimer::singleShot(window ? 500 : 0, this, done);
}

void ChessConnector::pageOpened()
{
	if (!_gameOver)
		_clock->resume();
}

void ChessConnector::setMoveTiming(int slideMs)
{
	auto timing = _gate->timing();
	timing.slideMs = slideMs;
	_gate->setTiming(timing);
}

bool ChessConnector::isBoardIdle() const
{
	return _gate->isIdle();
}

QString ChessConnector::robotState() const
{
	return QStringLiteral("robot: %1, thinking %2, request %3, auto-play %4; gate: holding %5, board settled %6, idle %7; "
	                      "%8 plies, %9, review %10")
		.arg(_engineWorker ? QStringLiteral("running") : QStringLiteral("none"))
		.arg(_engineThinking).arg(_engineRequest).arg(_engineAutoPlay)
		.arg(_gate->holding()).arg(_gate->stageReady()).arg(_gate->isIdle())
		.arg(_game->GetMoveCount())
		.arg(_gameResult.isEmpty() ? QStringLiteral("no result") : _gameResult)
		.arg(reviewMode());
}

void ChessConnector::onEngineMoveError(int request, const QString& message, const QString& details)
{
	if (request != _engineRequest) {
		ENGINE_TRACE("error to request" << request << "dropped: waiting for" << _engineRequest
			<< ", thinking" << _engineThinking);
		return;
	}
	_engineThinking = false;
	emit engineThinkingChanged();
	_clock->pause(); // no answer is coming: that's no win on time. Undo goes on with the game
	qDebug() << "[Engine] move error:" << message;

	// One log line per dump line so the board and the move list stay readable in the log file.
	for (const auto& line : details.split('\n', Qt::SkipEmptyParts))
		qDebug().noquote() << "[Engine]" << line;

	if (_resignAfterEngineMove) {
		_resignAfterEngineMove = false;
		resignGame();
	}
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
			// The robot's answer may be waiting in the gate or still on its way: the takeback
			// drops it, so stop waiting for it (nothing else would clear the thinking state).
			// A resignation confirmed while it thought is cancelled by the takeback.
			if (_engineThinking)
				dropRobotAnswer();
			if (_promotionPending) return;
			cancelPremove(); // it was for the position being taken back
			int count = _game->GetMoveCount() - _forkPly; // Play from here: the game's first moves stay
			if (count == 0) {
				// Nothing to take back. If the answer just dropped was the robot's first move (or
				// its re-asked first move), ask again; otherwise the robot would never move.
				if (_game->IsWhiteMove() != _config.playerPlaysWhite)
					askEngineMove();
				emit noMoreMovesNotify();
				return;
			}
			// Take back until it is the player's turn: normally the robot's reply and the
			// player's move; after the player mated the robot, only the player's move.
			const bool robotToMove = _game->IsWhiteMove() != _config.playerPlaysWhite;
			int movesToUndo = (robotToMove || count < 2) ? 1 : 2;
			for (int i = 0; i < movesToUndo; i++) {
				if (i > 0 || !_gameOver) // the move that ended the game got no increment
					_clock->moveTakenBack(!_game->IsWhiteMove(), clockPly());
				_game->UndoMove();
			}
			// Taking back the last moves of a finished game (mate or resign) resumes it.
			if (_gameOver) {
				_gameOver = false;
				setFlaggedKing(-1);
				if (!_gameResult.isEmpty()) setResult({}, {});
			}
			_clock->resume();
			_clock->setTurn(_game->IsWhiteMove(), clockPly());
			buildFullHistoryCache();
			clearPossibleMoves();
			EmitMoveCountUpdates();
			++_undoCount;
			updateRecord();
			_gate->moveShown(); // the undo slides the pieces back: the next answer waits for it
			// E.g. the robot's own first move was taken back: let it play again.
			if (_game->IsWhiteMove() != _config.playerPlaysWhite) {
				askEngineMove();
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

// _reviewIndex changed: show that position (Pieces follows shownGame()).
void ChessConnector::emitBoardState() {
	schedulePiecesSync();
	clearPossibleMoves();
	emit lastMoveChanged();
}

void ChessConnector::reviewFirst() {
	if (_player) {
		while (_player->CanMove(false)) _player->MoveBack();
		EmitMoveCountUpdates(false);
		return;
	}
	showHeldMoves();
	_reviewIndex = 0;
	emitBoardState();
	emit reviewStateChanged();
}

void ChessConnector::reviewPrev() {
	if (_player) {
		if (!_player->CanMove(false)) { emit noMoreMovesNotify(); return; }
		_player->MoveBack();
		EmitMoveCountUpdates(false);
		return;
	}
	showHeldMoves();
	int cur = reviewMode() ? _reviewIndex : (int)_game->GetGameRecord().size();
	if (cur <= 0) return;
	_reviewIndex = cur - 1;
	emitBoardState();
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
	emitBoardState();
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
	emitBoardState();
	// The jump back to the live position animates the pieces: a held robot answer (the board wasn't
	// settled while reviewing) waits for it, and so does a premove kept during the review.
	_gate->moveShown();
	if (_premoveFrom >= 0)
		_premoveTimer.start(_gate->timing().slideMs);
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
	showHeldMoves();
	_reviewIndex = std::max(0, ply);
	emitBoardState();
	emit reviewStateChanged();
}

int ChessConnector::IsOnPlayerMode()
{
	return _player ? 1 : 0;
}

void ChessConnector::resignAndLeave()
{
	if (!isOnlineGame() || !_gameResult.isEmpty()) {
		endGame();
		return;
	}
	resignGame();
	_leaveAfterResign.start(5000); // without an answer the loss isn't recorded here; Lichess has it
}

void ChessConnector::withdrawRematch()
{
	if (_rematchChallengeId.isEmpty())
		return;
	const QString challengeId = std::exchange(_rematchChallengeId, QString());
	if (_onlineClient) _onlineClient->cancelChallenge(challengeId);
}

void ChessConnector::endGame()
{
	// BL-36: our rematch challenge goes with the game it was made from (first: cancelChallenge stops the
	// stream too).
	withdrawRematch();
	_startTiming.cancel(); // e.g. a Lichess game that couldn't be played here, left before its first frame
	_leaveAfterResign.stop(); // also when the game is left another way (Home again, switch user)
	cancelPremove();
	resetMoveHistoryCache();
	setFlaggedKing(-1);
	_resignAfterEngineMove = false;
	setPendingOffer({});
	setOpponentGone(false);
	setReconnecting(false);
	keepGameOnLeave(); // an unfinished local game stays in the library as it is: Continue offers it
	_clock->stop();
	if (!_onlineGameId.isEmpty()) {
		if (_onlineClient) _onlineClient->stopStream();
		_onlineGameId.clear();
		_afterHeldMoves.clear();
		_listAfterPromotion.reset();
		emit isOnlineGameChanged();
	}

	stopEngineThread();
	_promotionPending = false;
	_player = nullptr;
	restartGame(Chess::StartPosition()); // Home: no game, the standard board
	_gameOver = false;
	_reviewIndex = -1;
	if (!_gameResult.isEmpty()) setResult({}, {});
	EmitMoveCountUpdates();
	emit IsOnPlayerModeChanged();
	clearPossibleMoves();
	_recordId.clear();
	_replayResult.clear();
	_keptLichessGameId.clear();
	setGameMode({}, {});
}

void ChessConnector::switchUser()
{
	endGame(); // keeps the game for the user who played it
	// LI-09: a quick pairing seek is this profile's. The profile picker keeps its Lichess token, so a
	// new token wouldn't end it, and a game paired meanwhile would start behind the picker.
	if (_onlineClient)
		_onlineClient->cancelSeek();
	if (_userManager)
		_userManager->logout();
}

// BL-19: the library's record of the game on the board. Made at its first move (a game without one
// isn't kept), then written with every move, takeback, the result and when it is left; a Lichess game
// only once its result came (LI-14); a replay has none.
void ChessConnector::updateRecord()
{
	if (!_library)
		return;
	if (isOnlineGame()) {
		keepLichessGame();
		return;
	}
	if (_gameMode != QLatin1String("computer") && _gameMode != QLatin1String("local"))
		return;
	const auto& record = _game->GetGameRecord();
	if ((int)record.size() <= _forkPly && _recordId.isEmpty())
		return; // no move of its own yet (a fork: past the moves it began with)
	GameRecord r;
	if (!_recordId.isEmpty())
		if (const auto kept = _library->find(_recordId))
			r = *kept;
	r.id = _recordId;
	r.kind = _wasVsComputer ? QStringLiteral("computer") : QStringLiteral("local");
	r.finished = !_gameResult.isEmpty();
	r.startedAt = _recordStarted;
	if (!r.finished)
		r.finishedAt = QDateTime();
	else if (!r.finishedAt.isValid())
		r.finishedAt = QDateTime::currentDateTime();
	r.playerPlaysWhite = _config.playerPlaysWhite;
	r.opponentName = _opponentName;
	r.opponentUserId = _opponentUserId;
	r.level = _wasVsComputer ? _config.lastLevel : 0;
	if (_avatarProvider) {
		r.playerAvatar = _avatarProvider->playerRawName();
		r.opponentAvatar = _avatarProvider->opponentRawName();
	}
	if (_clock->enabled()) {
		r.clockLimitSec = _clockLimitSec;
		r.clockIncSec = _clockIncSec;
		r.whiteMs = _clock->remainingMs(true);
		r.blackMs = _clock->remainingMs(false);
	} else {
		r.clockLimitSec = r.clockIncSec = 0;
		r.whiteMs = r.blackMs = 0;
	}
	setRecordPosition(r);
	r.result = r.finished ? _gameResult.section(' ', 0, 0) : QString();
	r.resultText = r.finished ? _resultText : QString();
	r.resultRecorded = _resultRecorded;
	r.undoCount = _undoCount;
	r.forkOfId = _forkOfId;
	r.forkPly = _forkPly;
	_recordId = _library->put(r);
}

void ChessConnector::setRecordPosition(GameRecord& r) const
{
	r.setStartPosition(_game->GetStartPosition());
	r.moves.clear();
	for (const auto& m : _game->GetGameRecord())
		r.moves.append(QString::fromStdString(m.ToUciString()));
	r.fen = QString::fromStdString(_game->MakeFen());
}

// LI-14: a Lichess game is kept once its result came here (the owner's decision), never while it goes on, nor
// an aborted one (Lichess doesn't count it either). One record per Lichess game, lichess-<id>: written again
// (a game joined after it ended) it replaces itself, its analysis kept while its moves are the same.
void ChessConnector::keepLichessGame()
{
	const QString result = _gameResult.section(' ', 0, 0);
	if (result.isEmpty() || result == QLatin1String("*") || _onlineGameId.isEmpty() || plyCount() == 0)
		return;
	GameRecord r;
	r.id = GameRecord::lichessRecordId(_onlineGameId);
	if (const auto kept = _library->find(r.id))
		r = *kept;
	r.kind = QStringLiteral("lichess");
	r.finished = true;
	const qint64 created = _onlineGameFull.value(QStringLiteral("createdAt")).toInteger();
	r.startedAt = created > 0 ? QDateTime::fromMSecsSinceEpoch(created) : _recordStarted;
	if (!r.finishedAt.isValid())
		r.finishedAt = QDateTime::currentDateTime();
	r.playerPlaysWhite = _config.playerPlaysWhite;
	r.opponentName = _opponentName;
	r.opponentUserId.clear();
	r.level = 0;
	if (_avatarProvider)
		r.playerAvatar = _avatarProvider->playerRawName();
	const QJsonObject clock = _onlineGameFull.value(QStringLiteral("clock")).toObject();
	r.clockLimitSec = clock.value(QStringLiteral("initial")).toInt() / 1000;
	r.clockIncSec = clock.value(QStringLiteral("increment")).toInt() / 1000;
	r.whiteMs = r.blackMs = 0; // nothing to go on with
	setRecordPosition(r);
	r.result = result;
	r.resultText = _resultText;
	r.resultRecorded = _resultRecorded;
	const bool white = _config.playerPlaysWhite;
	r.lichess = LichessGameInfo(_onlineGameId,
		_onlineGameFull.value(QStringLiteral("rated")).toBool(),
		_onlineGameFull.value(QStringLiteral("speed")).toString(),
		_onlineGameFull.value(QStringLiteral("daysPerTurn")).toInt(),
		white ? _lichessMe : _lichessOpponent, white ? _lichessOpponent : _lichessMe);
	_recordId = r.id; // before put(): its gamesChanged refreshes the board's analysis, which looks for it
	_library->put(r);
}

// Leaving (Home, switch user, closing the app): the game as it stands. A pawn still waiting for its
// piece hasn't moved yet.
void ChessConnector::keepGameOnLeave()
{
	if (_promotionPending && _game->GetMoveCount() > 0) {
		_promotionPending = false;
		_game->UndoMove();
	}
	updateRecord();
}

void ChessConnector::replayMoves(const QStringList& uciMoves)
{
	_resuming = true;
	const auto resumed = qScopeGuard([this] { _resuming = false; });
	if (!GameRecord::playOn(*_game, uciMoves))
		qWarning() << "replayMoves: a kept move can't be played; the game stops before it";
}

// An unfinished kept game goes on: vs the robot at the game's own level (lastLevel may have changed
// since: another profile), or two players with the opponent's profile when it still exists (else as
// Guest, under the name it had). The times as they were left, paused until the page is open (pageOpened).
bool ChessConnector::continueGame(const QString& gameId)
{
	const auto rec = _library ? _library->find(gameId) : std::nullopt;
	if (!rec || rec->finished || rec->isLichess() || rec->moves.isEmpty()) // a Lichess game goes on on Lichess (LI-04)
		return false;
	// All of its moves, or the game isn't touched: going on from fewer would keep only those (the record
	// is written again with the next move).
	if (!rec->replay()) {
		qWarning() << "continueGame: a kept move of" << gameId << "can't be played";
		emit keptGameFailed(QStringLiteral("Couldn't continue the game: one of its moves can't be played."));
		return false;
	}
	const bool robot = rec->kind == QLatin1String("computer");
	_startTiming.begin(robot ? QStringLiteral("continue vs Robot") : QStringLiteral("continue two players"));

	QString opponentName = rec->opponentName.isEmpty() ? (robot ? QStringLiteral("Robot") : QStringLiteral("Guest")) : rec->opponentName;
	QString opponentId;
	if (!robot && !rec->opponentUserId.isEmpty() && _userManager) {
		const QString profileName = _userManager->nameOf(rec->opponentUserId);
		if (!profileName.isEmpty()) { // still there (perhaps renamed); else the name it had, as a Guest
			opponentName = profileName;
			opponentId = rec->opponentUserId;
		}
	}

	resetMoveHistoryCache();
	_wasVsComputer = robot;
	beginGame(false);
	_recordId = rec->id;
	_recordStarted = rec->startedAt;
	_undoCount = rec->undoCount;
	_forkOfId = rec->forkOfId;
	_forkPly = rec->forkPly;
	_gameDate = rec->startedAt.date();
	_resultRecorded = rec->resultRecorded; // mated, undone and left: the mate counted already
	if (robot && rec->level > 0 && rec->level != _config.lastLevel) {
		_config.lastLevel = rec->level;
		emit lastLevelChanged();
	}
	_config.playerPlaysWhite = rec->playerPlaysWhite;
	emit playerPlaysWhiteChanged();
	if (_avatarProvider)
		_avatarProvider->restore(rec->playerAvatar, rec->opponentAvatar, robot);

	stopEngineThread();
	_startTiming.step("stop engine");
	_promotionPending = false;
	_player = nullptr;
	_reviewIndex = -1;
	restartGame(rec->startPosition());
	replayMoves(rec->moves);
	_gameOver = false;
	if (!_gameResult.isEmpty()) setResult({}, {});

	_clockLimitSec = rec->clockLimitSec;
	_clockIncSec = rec->clockIncSec;
	startClock();
	if (_clock->enabled()) {
		_clock->pause();
		_clock->setSnapshot(rec->whiteMs, rec->blackMs, _game->IsWhiteMove(), clockPly() >= 2);
	}

	buildFullHistoryCache();
	_startTiming.step("load");
	EmitMoveCountUpdates();
	emit IsOnPlayerModeChanged();

	if (robot) {
		_engineAutoPlay = true;
		startEngineThread(Chess::EngineLevel(_config.lastLevel));
		if (_game->IsWhiteMove() != _config.playerPlaysWhite && !_promotionPending)
			askEngineMove();
		_startTiming.step("start engine");
	}
	setGameMode(robot ? QStringLiteral("computer") : QStringLiteral("local"), opponentName, opponentId);
	return true;
}

// Play from here (BL-19 step 4): the kept game's first `ply` moves, when they can be played and the
// game isn't over after them.
std::optional<ChessConnector::StartFrom> ChessConnector::startFrom(const QString& gameId, int ply) const
{
	const auto rec = _library ? _library->find(gameId) : std::nullopt;
	if (!rec || rec->isLichess() || ply < 1 || ply > rec->plyCount()) // LI-14: a Lichess game isn't Played from here
		return std::nullopt;
	const QStringList moves = rec->moves.mid(0, ply);
	const auto game = rec->replay(ply);
	if (!game || game->IsCheckMate() || game->IsStalemate())
		return std::nullopt;
	return StartFrom{ rec->id, ply, moves, rec->startPosition() };
}

QVariantMap ChessConnector::playFromHere() const
{
	// Only while reviewing (step 4b): a replay, or a robot / two-player game stepped back once it is
	// over; a game still going on isn't forked (the owner, 2026-11-17).
	const bool replay = _gameMode == QLatin1String("replay");
	const bool local = _gameMode == QLatin1String("computer") || _gameMode == QLatin1String("local");
	if (isOnlineGame() || !(replay || (local && reviewMode() && !_gameResult.isEmpty())))
		return {};
	const int ply = shownPly();
	const auto from = startFrom(_recordId, ply);
	if (!from)
		return {};
	return QVariantMap{
		{ QStringLiteral("gameId"),      from->gameId },
		{ QStringLiteral("ply"),         ply },
		{ QStringLiteral("move"),        shownMove() },
		{ QStringLiteral("whiteToMove"), ply % 2 == 0 },
	};
}

bool ChessConnector::startComputerGameFrom(const QString& gameId, int ply, int level, int side, int limitSec, int incSec)
{
	_startFrom = startFrom(gameId, ply);
	if (!_startFrom)
		return false;
	startComputerGame(level, side, limitSec, incSec); // the moves go on the board after its restart
	return true;
}

bool ChessConnector::startLocalGameFrom(const QString& gameId, int ply, bool playerPlaysWhite, const QString& opponentUserId,
                                        int limitSec, int incSec)
{
	_startFrom = startFrom(gameId, ply);
	if (!_startFrom)
		return false;
	startLocalGame(playerPlaysWhite, opponentUserId, limitSec, incSec);
	return true;
}

void ChessConnector::applyStartFrom()
{
	if (!_startFrom)
		return;
	const StartFrom from = std::move(*_startFrom);
	_startFrom.reset();
	replayMoves(from.moves);
	_forkOfId = from.gameId;
	_forkPly = _game->GetMoveCount();
	buildFullHistoryCache();
}

int ChessConnector::clockPly() const
{
	return std::max(0, _game->GetMoveCount() - _forkPly);
}

// A kept game as a replay: the board at the start position, then its moves one by one (HistoryPlayer).
bool ChessConnector::openGame(const QString& gameId)
{
	const auto rec = _library ? _library->find(gameId) : std::nullopt;
	if (!rec || rec->moves.isEmpty())
		return false;
	if (!rec->replay()) {
		qWarning() << "openGame: a kept move of" << gameId << "can't be played";
		emit keptGameFailed(QStringLiteral("Couldn't open the game: one of its moves can't be played."));
		return false;
	}
	_startTiming.begin(QStringLiteral("open a kept game"));
	beginGame(false); // nothing of the game before carries over (its fork, takebacks, a Lichess rematch)
	_wasVsComputer = rec->kind == QLatin1String("computer");
	_clock->stop();
	stopEngineThread();
	resetMoveHistoryCache();
	_config.playerPlaysWhite = rec->playerPlaysWhite;
	emit playerPlaysWhiteChanged();
	if (_avatarProvider)
		_avatarProvider->restore(rec->playerAvatar, rec->opponentAvatar, rec->kind == QLatin1String("computer"));
	if (rec->isLichess()) {
		// LI-14: the cards show the Lichess players as the game did (LI-07), the opponent's flair as their avatar.
		_keptLichessGameId = rec->lichess.gameId();
		_lichessMe       = rec->lichess.player(rec->playerPlaysWhite);
		_lichessOpponent = rec->lichess.player(!rec->playerPlaysWhite);
		if (_avatarProvider)
			_avatarProvider->setOpponentOnline(_lichessOpponent.flairUrl());
	}
	_promotionPending = false;
	_player = nullptr;
	std::vector<std::string> uci;
	for (const auto& m : rec->moves)
		uci.push_back(m.toStdString());
	try {
		_resuming = true;
		_loadingRecord = true;
		const auto done = qScopeGuard([this] { _resuming = false; _loadingRecord = false; });
		restartGame(rec->startPosition()); // LoadMoves plays them from the game's start
		_game->LoadMoves(uci);
	}
	catch (const std::exception& ex) {
		qWarning() << "openGame: the kept moves can't be played:" << ex.what();
		restartGame(Chess::StartPosition());
		return false;
	}
	_player = _game->MakePlayer();
	_gameDate = rec->startedAt.date();
	_replayResult = rec->result;
	_recordId = rec->id;
	_gameOver = false;
	_reviewIndex = -1;
	setFlaggedKing(-1);
	if (!_gameResult.isEmpty()) setResult({}, {});
	buildFullHistoryCache();
	_startTiming.step("load");
	emit IsOnPlayerModeChanged();
	EmitMoveCountUpdates();
	setGameMode(QStringLiteral("replay"), rec->opponentName, rec->opponentUserId);
	return true;
}

ChessConnector::~ChessConnector()
{
	keepGameOnLeave(); // closing the app during a local game: Continue offers it next time

	stopEngineThread();
	waitForStoppedEngines();
	qDebug() << "Game Exited.";
}

