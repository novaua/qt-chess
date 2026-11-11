#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QDate>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include "MovesGen.h"
#include <QThread>
#include <QTimer>
#include <functional>
#include <optional>
#include <utility>

#include "Game.h"
#include "boardpieces.h"
#include "movelistmodel.h"
#include "movegate.h"
#include "gameclock.h"
#include "claimwin.h"
#include "engineworker.h"
#include "AppConfig.h"
#include "AvatarProvider.h"
#include "UserManager.h"
#include "LichessClient.h"

class ChessConnector : public QObject
{
	Q_OBJECT
		Q_PROPERTY(int MoveCount READ MoveCount NOTIFY moveCountChanged)
		Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
		// The pieces of the position on the board (the reviewed one while reviewing), one row per
		// piece that keeps its id while it moves: { pieceId, square, piece, taken } (BoardPieces).
		Q_PROPERTY(QAbstractItemModel* Pieces READ pieces CONSTANT)
		Q_PROPERTY(int SelectedSquare READ selectedSquare NOTIFY PossibleMovesChanged) // the piece whose moves show, or -1
		Q_PROPERTY(int MovableColor READ movableColor NOTIFY movableColorChanged) // whose pieces the player can move (or premove) now: 0 none, 1 White, 2 Black
		// LI-05: a move picked during the robot's or the Lichess opponent's turn, played when it comes
		// if it is legal then. Its two squares, or -1. Hidden (-1) while reviewing: it is kept, and played
		// once the board is back at the live position.
		Q_PROPERTY(int PremoveFrom READ premoveFrom NOTIFY premoveChanged)
		Q_PROPERTY(int PremoveTo   READ premoveTo   NOTIFY premoveChanged)
		Q_PROPERTY(bool PremoveDots READ premoveDots NOTIFY PossibleMovesChanged) // the dots are where a premove may go
		Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
		Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
		Q_PROPERTY(bool CanContinue READ canContinue NOTIFY canContinueChanged)
		Q_PROPERTY(bool CanLoad READ canLoad NOTIFY canLoadChanged)
		Q_PROPERTY(int LastLevel    READ lastLevel    NOTIFY lastLevelChanged)
		Q_PROPERTY(QVariantList Levels READ levels CONSTANT)
		Q_PROPERTY(int LastMoveFrom READ lastMoveFrom NOTIFY lastMoveChanged)
		Q_PROPERTY(int LastMoveTo   READ lastMoveTo   NOTIFY lastMoveChanged)
		// Pieces taken in the position on the board, in value order P N B R Q: "P" white, "p" black.
		Q_PROPERTY(QStringList CapturedByDark  READ capturedByDark  NOTIFY capturedChanged)
		Q_PROPERTY(QStringList CapturedByLight READ capturedByLight NOTIFY capturedChanged)
		Q_PROPERTY(int MaterialBalance READ materialBalance NOTIFY capturedChanged) // White minus Black, in pawns
		Q_PROPERTY(bool EngineThinking    READ engineThinking    NOTIFY engineThinkingChanged)
		Q_PROPERTY(bool UseFen            READ useFen            WRITE setUseFen NOTIFY useFenChanged)
		Q_PROPERTY(bool PlayerPlaysWhite  READ playerPlaysWhite  NOTIFY playerPlaysWhiteChanged)
		// One row per move number: { n, wPly, wPiece, wText, bPly, bPiece, bText } (MoveListModel);
		// a move changes one row, not the whole list.
		Q_PROPERTY(QAbstractItemModel* MoveHistory READ moveHistory CONSTANT)
		Q_PROPERTY(int  ShownPly      READ shownPly      NOTIFY reviewStateChanged) // moves on the board now
		Q_PROPERTY(QString ShownMove  READ shownMove     NOTIFY reviewStateChanged) // the move on the board, e.g. "12. Nf3", "12… exd5"; "" at the start
		Q_PROPERTY(int  PlyCount      READ plyCount      NOTIFY moveHistoryChanged) // moves in the list
		Q_PROPERTY(QString      GameResult  READ gameResult  NOTIFY gameResultChanged)
		Q_PROPERTY(bool ReviewMode    READ reviewMode    NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewPrev READ canReviewPrev NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewNext READ canReviewNext NOTIFY reviewStateChanged)
		Q_PROPERTY(bool IsOnlineGame  READ isOnlineGame  NOTIFY isOnlineGameChanged)
		Q_PROPERTY(bool CanRematch    READ canRematch    NOTIFY canRematchChanged)
		Q_PROPERTY(int  FlaggedKing   READ flaggedKing   NOTIFY flaggedKingChanged) // square of a king that resigned / lost on time, or -1
		Q_PROPERTY(int  CheckedKing   READ checkedKing   NOTIFY lastMoveChanged)    // square of the king in check on the board, or -1
		// The game on screen: "" (none: Home), "computer", "local" (two players), "replay" (saved game), "online"
		Q_PROPERTY(bool    InGame    READ inGame    NOTIFY gameInfoChanged)
		Q_PROPERTY(QString GameMode  READ gameMode  NOTIFY gameInfoChanged)
		Q_PROPERTY(QString GameTitle READ gameTitle NOTIFY gameInfoChanged) // e.g. "vs Robot · Level 3", "Alice vs Bob"
		Q_PROPERTY(QString PlayerName   READ playerName   NOTIFY gameInfoChanged) // the bottom player card
		Q_PROPERTY(QString OpponentName READ opponentName NOTIFY gameInfoChanged) // the top one
		Q_PROPERTY(QString ResultText READ resultText NOTIFY gameResultChanged) // e.g. "Bob wins by checkmate"
		Q_PROPERTY(QString PendingOffer READ pendingOffer NOTIFY pendingOfferChanged) // Lichess opponent offers: "", "draw", "takeback", "rematch"
		Q_PROPERTY(bool CanOfferDraw    READ canOfferDraw    NOTIFY canOfferDrawChanged) // Lichess: from ply 2, then 20 plies after our last offer
		Q_PROPERTY(bool OpponentGone    READ opponentGone    NOTIFY opponentGoneChanged)
		Q_PROPERTY(bool Reconnecting    READ reconnecting    NOTIFY reconnectingChanged) // LI-06: the Lichess game's connection is lost
		// BL-10: a challenge can be accepted now: not during a running real-time Lichess game (the
		// player finishes it first); a robot, two-player or correspondence game is left for it.
		Q_PROPERTY(bool CanAcceptChallenge READ canAcceptChallenge NOTIFY canAcceptChallengeChanged)
		// The opponent left: the seconds until Lichess allows claiming the win, counting down; -1 = not said.
		// CanClaimVictory: now, and on the opponent's turn, the only time Lichess takes the claim (ClaimWin).
		Q_PROPERTY(int  ClaimWinSeconds READ claimWinSeconds NOTIFY claimWinSecondsChanged)
		Q_PROPERTY(bool CanClaimVictory READ canClaimVictory NOTIFY canClaimVictoryChanged)
		Q_PROPERTY(QString ContinueDescription READ continueDescription NOTIFY canContinueChanged) // the autosaved game, e.g. "vs Bob"
		// BL-17: the autosaved robot game for its card in the Continue list: { level, playerPlaysWhite,
		// playerToMove, fen, lastMove (UCI) }; empty without one (CanContinue false).
		Q_PROPERTY(QVariantMap ContinueGame READ continueGameInfo NOTIFY canContinueChanged)
		// BL-17: an unfinished Lichess game is being rejoined (continueOnlineGame) until its game
		// starts, it fails (gameStartFailed) or it is dropped (cancelOnlineStart, another profile).
		Q_PROPERTY(bool StartingOnlineGame READ startingOnlineGame NOTIFY startingOnlineGameChanged)
		Q_PROPERTY(GameClock* Clock READ clock CONSTANT) // the game's clocks; Clock.enabled false: none
		// --screenshot mode: { scene, moves, black, dark, width, height }; empty in a normal run
		Q_PROPERTY(QVariantMap ScreenshotScene READ screenshotScene CONSTANT)
public:
	explicit ChessConnector(QObject* parent = nullptr);
	~ChessConnector();

	void setAvatarProvider(AvatarProvider* ap) { _avatarProvider = ap; }
	void setUserManager(UserManager* um);
	void setLichessClient(LichessClient* lc);
	void setScreenshotScene(const QVariantMap& scene);

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();
	bool canContinue() const;
	bool canLoad() const;
	int lastLevel()        const { return _config.lastLevel; }
	QVariantList levels()  const; // computer levels 1-5: { level, name, elo, eloEstimated, rating, hint }
	int  lastMoveFrom()   const;
	int  lastMoveTo()     const;
	bool engineThinking()    const { return _engineThinking; }
	bool useFen()            const { return _config.useFen; }
	bool playerPlaysWhite()  const { return _config.playerPlaysWhite; }
	QAbstractItemModel* moveHistory() const { return _moveList; }
	QString shownMove() const;
	QString      gameResult()  const { return _gameResult; }
	bool reviewMode()    const { return _reviewIndex >= 0; }
	int  shownPly()      const;
	int  plyCount()      const { return (int)_sanCache.size(); }
	bool canReviewPrev() const;
	bool canReviewNext() const;
	bool isOnlineGame()  const { return !_onlineGameId.isEmpty(); }
	// Local games can always be replayed; a Lichess game needs its settings and a token.
	bool canRematch()    const { return !_lastGameOnline || (_lichessClient && _lichessClient->canRematch()); }
	// Hidden while reviewing earlier moves: the king may not be on that square then.
	int  flaggedKing()   const { return reviewMode() ? -1 : _flaggedKing; }
	int  checkedKing()   const;
	bool    inGame()     const { return !_gameMode.isEmpty(); }
	QString gameMode()   const { return _gameMode; }
	QString gameTitle()  const;
	QString playerName() const;
	QString opponentName() const { return _opponentName; }
	QString resultText() const { return _resultText; }
	QString pendingOffer() const { return _pendingOffer; }
	bool    canOfferDraw() const;
	bool    opponentGone()    const { return _claimWin->gone(); }
	bool    reconnecting()    const { return _reconnecting; }
	bool    canAcceptChallenge() const { return !(isOnlineGame() && _gameResult.isEmpty() && _onlineRealTime); }
	int     claimWinSeconds() const { return _claimWin->secondsLeft(); }
	bool    canClaimVictory() const { return _claimWin->canClaim(); }
	QString continueDescription() const;
	QVariantMap continueGameInfo() const;
	bool startingOnlineGame() const { return _lichessClient && !_lichessClient->rejoiningGameId().isEmpty(); }
	QStringList capturedByDark()  const;
	QStringList capturedByLight() const;
	int materialBalance() const;
	QVariantMap screenshotScene() const { return _screenshotScene; }
	GameClock* clock() const { return _clock; }
	QAbstractItemModel* pieces() const { return _pieces; }
	int selectedSquare() const { return _selectedSquare; }
	int movableColor() const;
	int premoveFrom() const { return reviewMode() ? -1 : _premoveFrom; }
	int premoveTo()   const { return reviewMode() ? -1 : _premoveTo; }
	bool premoveDots() const { return _premoveDots; }

	QStringList& PossibleMoves();
	void setPossibleMoves(const QStringList& moves);

signals:
	void PossibleMovesChanged();
	void movableColorChanged();
	void premoveChanged();

	void moveCountChanged();
	void IsWhiteMoveChanged();
	void IsOnPlayerModeChanged();
	void canContinueChanged();
	void startingOnlineGameChanged();
	void gameStartFailed(QString message); // continueOnlineGame(): Lichess didn't give the game
	void canLoadChanged();
	void lastLevelChanged();

	void checkNotify();
	void checkMateNotify();
	void checkMateResult(QString winner);
	void engineThinkingChanged();
	void useFenChanged();
	void playerPlaysWhiteChanged();
	void lastMoveChanged();
	void capturedChanged();
	void newGameStarted(bool isComputerGame);

	void moveHistoryChanged();
	void gameResultChanged();
	void reviewStateChanged();
	void isOnlineGameChanged();
	void canRematchChanged();
	void flaggedKingChanged();
	void gameInfoChanged();
	void pendingOfferChanged();
	void canOfferDrawChanged();
	void opponentGoneChanged();
	void claimWinSecondsChanged();
	void canClaimVictoryChanged();
	void reconnectingChanged();
	void canAcceptChallengeChanged();
	void reconnected(); // the Lichess game's connection is back, the board up to date

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);
	void pawnPromotionCancelled(); // the time ran out while the piece was being chosen

	void noSavedGame();
	void savedOk();
	void noMoreMovesNotify();
	void pgnCopied();
	void pgnExported(bool ok, const QString& path);
	void movesTakenBack(int plies); // a Lichess takeback was accepted (by either side)

	void screenshotSceneReady(); // QML has set up the --screenshot scene: grab the window
	void screenshotSceneSetUp(); // the takeback scene: QML is done, C++ plays the rest (main.cpp)
	void boardIdle();            // the page has settled and no move waits to be shown (BL-14)

public slots:
	// One call per user action (CLAUDE.md "Key Conventions"). A Lichess game starts and ends
	// through LichessClient::gameStarted / gameEnded, handled below.
	// limitSec, incSec: the time control; 0 and 0: no clock.
	Q_INVOKABLE void startComputerGame(int level, int side, int limitSec = 0, int incSec = 0); // side: 0 White, 1 Black, 2 Random
	Q_INVOKABLE void startLocalGame(bool playerPlaysWhite, const QString& opponentUserId, int limitSec = 0, int incSec = 0); // "" = Guest
	Q_INVOKABLE bool continueGame();
	// BL-17: Home's Continue. The only unfinished game starts: the robot's autosave, or the one
	// Lichess game (when it can be played here). False with none, two or more, or while the Lichess
	// list is loading: the Continue list shows them then.
	Q_INVOKABLE bool continueOnlyGame();
	// Rejoin an unfinished Lichess game (the Continue list, the Online page): StartingOnlineGame, then
	// the game page, or gameStartFailed. False without a linked account.
	Q_INVOKABLE bool continueOnlineGame(const QString& gameId);
	Q_INVOKABLE void cancelOnlineStart(); // drop a rejoin under way (Back from its page)
	void setUseFen(bool v);
	Q_INVOKABLE void applyMoves(const QString& movesStr);
	Q_INVOKABLE void setScreenshotClock(int whiteMs, int blackMs); // --screenshot: the times to show; the side to move runs
	// The game page has settled (open for a second, the board not turning): a held move may show.
	Q_INVOKABLE void setBoardSettled(bool settled);
	Q_INVOKABLE void pageOpened(); // the game page has been open for a second: a continued game's clock starts
	// Theme.slow, or 0 with Reduce animations: how long a piece slides.
	Q_INVOKABLE void setMoveTiming(int slideMs);
	// The gate: the page has settled, it shows the live position, and no move waits to be shown.
	Q_INVOKABLE bool isBoardIdle() const;

	void endGame();              // leave the game for Home (autosaves an unfinished local game)
	// The leave warning's Resign: resign the Lichess game, then endGame() once Lichess has ended it
	// (so the loss counts), or after a few seconds without an answer. A finished game: endGame().
	Q_INVOKABLE void resignAndLeave();
	Q_INVOKABLE void switchUser(); // endGame(), then log out

	void figureSelected(int index); // a click: show the piece's moves, or play the move to that square
	Q_INVOKABLE bool movePiece(int from, int to); // a drop: plays it if legal, else false (the piece goes back)
	Q_INVOKABLE void cancelPremove(); // a right click or Esc: no premove, no premove dots
	void pawnPromote(int index, const QString& piece);

	void saveGame();
	bool loadGame();

	void moveNext();
	void movePrev();

	void reviewFirst();
	void reviewPrev();
	void reviewNext();
	void reviewLast();
	Q_INVOKABLE void reviewTo(int ply); // show the position after that many moves; the last = live

	Q_INVOKABLE QString pgn() const;            // the whole game with tags, also while reviewing
	Q_INVOKABLE QString pgnFileName() const;    // suggested name, e.g. "Alice vs Bob 2026-10-29.pgn"
	Q_INVOKABLE void copyPgn();
	Q_INVOKABLE void exportPgn(const QUrl& file);

	Q_INVOKABLE void resignGame();
	Q_INVOKABLE void offerDraw();
	Q_INVOKABLE void claimVictory(); // Lichess: after the opponent left the game
	Q_INVOKABLE void respondDraw(bool accept);
	Q_INVOKABLE void requestTakeback();
	Q_INVOKABLE void respondTakeback(bool accept);
	Q_INVOKABLE void respondRematch(bool accept);
	Q_INVOKABLE void rematch();
	// BL-10: accept a Lichess challenge to the player. The game on the board stays until Lichess has
	// started the new one (then a robot game is autosaved, a correspondence game goes on at Lichess),
	// so a refused Accept (LichessClient::challengeAcceptFailed) loses nothing. The game starts
	// through LichessClient::gameStarted, as for a link. False (nothing sent) while
	// CanAcceptChallenge is false.
	Q_INVOKABLE bool acceptChallenge(const QString& challengeId);

private slots:
	void onEngineMoveFound(int request, const QString& move);
	void onEngineMoveError(int request, const QString& message, const QString& details);
	void onLichessGameStarted(const QString& gameId, bool playingAsWhite, const QString& opponentName,
		const QString& opponentAvatar, const QStringList& moves);
	void onLichessGameEnded(const QString& status, const QString& winner);
	void onLichessOpponentGone(bool gone, int claimWinInSeconds);
	void onLichessClockStarted(qint64 limitMs, qint64 incrementMs, bool correspondence);
	void onLichessConnectionChanged(bool connected);
	void onClockFlagged(bool white); // a local game: that side's time is up

private:
	void startNewGame();
	void startNewGameWithComputer(int level);
	// joinedMoves (BL-18): the moves of a game joined in progress, on the board before its page opens.
	// False when one of them can't be played here.
	bool startOnlineGame(const QString& gameId, bool playingAsWhite, const QStringList& joinedMoves);
	void setPlayerPlaysWhite(bool v);
	// opponentUserId: the opponent's profile in a two-player game ("" = Guest, or not a local game).
	void setGameMode(const QString& mode, const QString& opponentName, const QString& opponentUserId = {});
	void recordResult(int whiteScore); // statistics; 1 White won, 0 draw, -1 Black won
	void setResult(const QString& result, const QString& text); // non-empty: the game is over
	void setPendingOffer(const QString& offer);
	void setOpponentGone(bool gone, int claimWinInSeconds = -1);
	void updateClaimTurn(); // tells _claimWin whose turn it is
	void setReconnecting(bool reconnecting);
	void flagKing(bool white); // mark that side's king as the loser (resign / time)
	void endGameWithLoser(bool white, const QString& result, const QString& text); // mate, resignation, time
	void dropRobotAnswer();    // on its way or waiting in the gate: nothing waits for it any more
	QString sideName(bool white) const; // who plays that colour
	QStringList capturedPieces(Chess::PieceColors capturedColor) const;
	const Chess::MovesHistory& shownHistory() const; // the saved game in replay, else the game record
	QString opponentNameOf(const UserManager::GameSaveInfo& info) const;
	void makeMove(int from, int to, Chess::PieceTypes promoteTo = Chess::EMPTY); // EMPTY: the player picks the piece
	bool canPlayerMove() const; // the player may move a piece now (not the robot's turn, not reviewing, ...)
	bool canPremove() const;    // the player may pick a premove now: the robot's or the Lichess opponent's turn
	bool isPlayerPiece(int square) const; // vs the robot or on Lichess: the player's own piece stands there
	void setPremove(int from, int to);
	void opponentMoveShown(); // the robot's or the Lichess opponent's move is on the board: a premove follows
	void playPremove();       // right after the opponent's move: the premove, if it is legal now
	void refreshSelection();  // the turn changed under a selected piece: its dots follow
	void clearPossibleMoves();  // no piece selected, no move dots
	std::shared_ptr<Chess::Game> shownGame() const; // the position on the board: a replay while reviewing
	void schedulePiecesSync();  // update Pieces once, after the current change is complete
	void onHumanMoveDone();
	void moveOnBoard(); // the player's or the robot's move is on the board: a stalemate, or the other clock runs
	void startClock();        // a new or continued local game: _clockLimitSec and _clockIncSec
	void playRobotMove(const QString& uciMove); // the gate released it: play it on the GUI thread
	void onGateRelease(const QString& uciMove, MoveGate::Source source);
	void onLichessOpponentMoves(const QString& uciMoves); // one move, or the whole game on a resync
	void onLichessMoveListReplaced(const QString& uciMoves, bool moveRefused); // a takeback, a refused move, a disagreement
	void playOnlineMoves(const QStringList& uciMoves); // the gate released them: play them
	// On the game, then history and counters; how many were played. announce: a check (or mate) by the
	// last one is told; false: none is (moves already played elsewhere).
	int  playMoves(const QStringList& uciMoves, bool announce = true);
	void afterHeldMoves(std::function<void()> action); // now, or once the opponent's held moves are shown
	void showHeldMoves(); // entering the review of a game on a clock: play the held moves now
	void emitBoardState(); // after _reviewIndex changed
	void EmitMoveCountUpdates(bool emitHistoryChanged = true);
	void buildFullHistoryCache();
	void appendMoveToHistory();
	void resetMoveHistoryCache();
	void startEngineThread(Chess::EngineLevel level = Chess::EngineLevel{ 3 });
	void stopEngineThread();
	void askEngineMove(); // the robot's turn: start the engine on a copy of the moves
	void beginGame(bool online); // resets per-game end state; online: rematch goes to Lichess
	void setFlaggedKing(int square);
	bool canAutoSave(); // an unfinished game vs the robot: the only kind Continue can resume
	void autoSaveGame();
	void deleteAutoSave();
	QString autoSavePath()  const;
	QString savedGamePath() const;

private:
	QStringList _possibleMoves;
	int _selectedSquare = -1;
	bool _premoveDots = false;
	int _premoveFrom = -1;
	int _premoveTo = -1;
	QTimer _premoveTimer; // back from a review: the jump to the live position slides, then the premove is played
	BoardPieces* _pieces = nullptr;
	bool _piecesSyncPosted = false;

	Chess::GameAptr _game;
	Chess::HistoryPlayerAptr _player;
	Chess::PawnPromotedCallback _onPawnPromotedCallback;

	AvatarProvider* _avatarProvider = nullptr;
	UserManager*    _userManager    = nullptr;
	LichessClient*  _lichessClient  = nullptr;
	QString         _onlineGameId;
	QThread* _engineThread = nullptr;
	EngineWorker* _engineWorker = nullptr;
	MoveGate* _gate = nullptr; // holds the robot's and the Lichess opponent's moves until the board is idle (BL-14)
	GameClock* _clock = nullptr;
	int _clockLimitSec = 0; // the local game's time control, kept for a rematch
	int _clockIncSec = 0;
	bool _piecesAnew = false;  // the next Pieces sync shows the position anew, without slides (a resync)
	std::vector<std::function<void()>> _afterHeldMoves; // e.g. a draw offer that came with the opponent's held move
	bool _engineThinking = false;
	int  _engineRequest  = 0; // the engine answer we wait for; older answers are dropped
	bool _engineAutoPlay = false;
	bool _promotionPending = false; // a pawn is on the last rank, waiting for pawnPromote()
	std::optional<std::pair<QString, bool>> _listAfterPromotion; // Lichess's moves that came meanwhile (and moveRefused)
	bool _gameOver = false;
	bool _wasVsComputer = false;
	bool _resuming = false; // Continue replays the saved moves: their checks were heard already
	bool _lastGameOnline = false; // the current/last game is a Lichess game: rematch goes online
	int  _flaggedKing    = -1;
	bool _resultRecorded = false; // stats recorded for this game (mate can recur after undo)
	bool _resignAfterEngineMove = false; // resign pressed while the robot was thinking
	QTimer _leaveAfterResign; // resignAndLeave(): runs until Lichess ends the game, then endGame()
	QString _gameResult;
	QString _resultText;
	QString _gameMode;
	QString _opponentName;
	QString _opponentUserId;
	QString _pendingOffer;
	int  _drawOfferPly = -1; // ply of our last Lichess draw offer; -1 = none in this game
	ClaimWin* _claimWin = nullptr; // the opponent left: the countdown and when the win can be claimed
	bool _reconnecting = false;
	bool _onlineRealTime = false; // the Lichess game on the board has a real-time clock
	AppConfig _config;
	int _reviewIndex = -1;

	MoveListModel* _moveList = nullptr;
	std::vector<std::string> _sanCache; // SAN of shownHistory(), for the PGN
	QDate _gameDate;                    // when the game started; invalid for a saved or continued game
	QVariantMap  _screenshotScene;
	bool hasAutoSave() const; // the file of a robot game; canContinue() also needs it readable
	// continueGameInfo(), read from the autosave once (empty: unreadable); reset by canContinueChanged.
	mutable std::optional<QVariantMap> _continueGameInfo;
	QString _onlineStartUserId; // the profile that started continueOnlineGame(): another one drops it
	void checkOnlineStart();    // the profile or its Lichess account changed: a rejoin of the other's is dropped
};

#endif // CHESSCONNECTOR_H
