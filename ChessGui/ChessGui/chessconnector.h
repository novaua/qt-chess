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
#include <atomic>
#include <functional>

#include "Game.h"
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
		// One row per move number: { n, wPly, wPiece, wText, bPly, bPiece, bText }. Ply = moves on the
		// board after that move (for reviewTo); piece = "N"/"n" etc. or "" for pawns and castling;
		// text = the SAN without the piece letter. No black move yet: bPly 0, bText "".
		Q_PROPERTY(QVariantList MoveHistory READ moveHistory NOTIFY moveHistoryChanged)
		Q_PROPERTY(int  ShownPly      READ shownPly      NOTIFY reviewStateChanged) // moves on the board now
		Q_PROPERTY(int  PlyCount      READ plyCount      NOTIFY moveHistoryChanged) // moves in the list
		Q_PROPERTY(QString      GameResult  READ gameResult  NOTIFY gameResultChanged)
		Q_PROPERTY(bool ReviewMode    READ reviewMode    NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewPrev READ canReviewPrev NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewNext READ canReviewNext NOTIFY reviewStateChanged)
		Q_PROPERTY(bool IsOnlineGame  READ isOnlineGame  NOTIFY isOnlineGameChanged)
		Q_PROPERTY(bool CanRematch    READ canRematch    NOTIFY canRematchChanged)
		Q_PROPERTY(int  FlaggedKing   READ flaggedKing   NOTIFY flaggedKingChanged) // square of a king that resigned / lost on time, or -1
		// The game on screen: "" (none: Home), "computer", "local" (two players), "replay" (saved game), "online"
		Q_PROPERTY(bool    InGame    READ inGame    NOTIFY gameInfoChanged)
		Q_PROPERTY(QString GameMode  READ gameMode  NOTIFY gameInfoChanged)
		Q_PROPERTY(QString GameTitle READ gameTitle NOTIFY gameInfoChanged) // e.g. "vs Robot · Level 3", "Alice vs Bob"
		Q_PROPERTY(QString PlayerName   READ playerName   NOTIFY gameInfoChanged) // the bottom player card
		Q_PROPERTY(QString OpponentName READ opponentName NOTIFY gameInfoChanged) // the top one
		Q_PROPERTY(QString ResultText READ resultText NOTIFY gameResultChanged) // e.g. "Bob wins by checkmate"
		Q_PROPERTY(QString PendingOffer READ pendingOffer NOTIFY pendingOfferChanged) // Lichess opponent offers: "", "draw", "takeback"
		Q_PROPERTY(bool OpponentGone    READ opponentGone    NOTIFY opponentGoneChanged)
		Q_PROPERTY(int  ClaimWinSeconds READ claimWinSeconds NOTIFY opponentGoneChanged) // when it became gone; -1 = not said
		Q_PROPERTY(QString ContinueDescription READ continueDescription NOTIFY canContinueChanged) // the autosaved game, e.g. "vs Bob"
		// --screenshot mode: { scene, moves, black, dark, width, height }; empty in a normal run
		Q_PROPERTY(QVariantMap ScreenshotScene READ screenshotScene CONSTANT)
public:
	explicit ChessConnector(QObject* parent = nullptr);
	~ChessConnector();

	void setAvatarProvider(AvatarProvider* ap) { _avatarProvider = ap; }
	void setUserManager(UserManager* um);
	void setLichessClient(LichessClient* lc);
	void setScreenshotScene(const QVariantMap& scene) { _screenshotScene = scene; }

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
	QVariantList moveHistory() const;
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
	bool    inGame()     const { return !_gameMode.isEmpty(); }
	QString gameMode()   const { return _gameMode; }
	QString gameTitle()  const;
	QString playerName() const;
	QString opponentName() const { return _opponentName; }
	QString resultText() const { return _resultText; }
	QString pendingOffer() const { return _pendingOffer; }
	bool    opponentGone()    const { return _opponentGone; }
	int     claimWinSeconds() const { return _claimWinSeconds; }
	QString continueDescription() const;
	QStringList capturedByDark()  const;
	QStringList capturedByLight() const;
	int materialBalance() const;
	QVariantMap screenshotScene() const { return _screenshotScene; }

	QStringList& PossibleMoves();
	void setPossibleMoves(const QStringList& moves);

signals:
	void boardChanged(int position, const QString& newValue);

	void PossibleMovesChanged();

	void moveCountChanged();
	void IsWhiteMoveChanged();
	void IsOnPlayerModeChanged();
	void canContinueChanged();
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
	void opponentGoneChanged();

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);

	void noSavedGame();
	void savedOk();
	void noMoreMovesNotify();
	void pgnCopied();
	void pgnExported(bool ok, const QString& path);

	void requestEngineMove();

	void screenshotSceneReady(); // QML has set up the --screenshot scene: grab the window

public slots:
	// One call per user action (CLAUDE.md "Key Conventions"). A Lichess game starts and ends
	// through LichessClient::gameStarted / gameEnded, handled below.
	Q_INVOKABLE void startComputerGame(int level, int side); // side: 0 White, 1 Black, 2 Random
	Q_INVOKABLE void startLocalGame(bool playerPlaysWhite, const QString& opponentUserId); // "" = Guest
	Q_INVOKABLE bool continueGame();
	void setUseFen(bool v);
	Q_INVOKABLE void applyMoves(const QString& movesStr);
	Q_INVOKABLE void refreshBoard(); // boardChanged for every square: a new board view fills itself

	void endGame();              // leave the game for Home (autosaves an unfinished local game)
	Q_INVOKABLE void switchUser(); // endGame(), then log out

	void figureSelected(int index);
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
	Q_INVOKABLE void rematch();

private slots:
	void onEngineMoveComplete();
	void onEngineMoveError(const QString& message, const QString& details);
	void onLichessGameStarted(const QString& gameId, bool playingAsWhite, const QString& opponentName,
		const QString& opponentAvatar);
	void onLichessGameEnded(const QString& status, const QString& winner);
	void onLichessOpponentGone(bool gone, int claimWinInSeconds);

private:
	void startNewGame();
	void startNewGameWithComputer(int level);
	void startOnlineGame(const QString& gameId, bool playingAsWhite);
	void setPlayerPlaysWhite(bool v);
	// opponentUserId: the opponent's profile in a two-player game ("" = Guest, or not a local game).
	void setGameMode(const QString& mode, const QString& opponentName, const QString& opponentUserId = {});
	void recordResult(int whiteScore); // statistics; 1 White won, 0 draw, -1 Black won
	void setResult(const QString& result, const QString& text); // non-empty: the game is over
	void setPendingOffer(const QString& offer);
	void setOpponentGone(bool gone, int claimWinInSeconds = -1);
	void flagKing(bool white); // mark that side's king as the loser (resign / time)
	QString sideName(bool white) const; // who plays that colour
	QStringList capturedPieces(Chess::PieceColors capturedColor) const;
	const Chess::MovesHistory& shownHistory() const; // the saved game in replay, else the game record
	QString opponentNameOf(const UserManager::GameSaveInfo& info) const;
	void makeMove(int from, int to);
	void onHumanMoveDone();
	void emitBoardState(int moveIndex);
	void EmitMoveCountUpdates(bool emitHistoryChanged = true);
	void addSanToRows(int i);
	void buildFullHistoryCache();
	void appendMoveToHistory();
	void resetMoveHistoryCache();
	void startEngineThread(Chess::EngineLevel level = Chess::EngineLevel{ 3 });
	void stopEngineThread();
	void beginGame(bool online); // resets per-game end state; online: rematch goes to Lichess
	void setFlaggedKing(int square);
	void autoSaveGame(bool isSinglePlayer);
	void deleteAutoSave();
	QString autoSavePath()  const;
	QString savedGamePath() const;

private:
	QStringList _possibleMoves;

	Chess::GameAptr _game;
	Chess::HistoryPlayerAptr _player;
	Chess::PawnPromotedCallback _onPawnPromotedCallback;

	AvatarProvider* _avatarProvider = nullptr;
	UserManager*    _userManager    = nullptr;
	LichessClient*  _lichessClient  = nullptr;
	QString         _onlineGameId;
	QThread* _engineThread = nullptr;
	EngineWorker* _engineWorker = nullptr;
	bool _engineThinking = false;
	bool _engineAutoPlay = false;
	bool _promotionPending = false; // a pawn is on the last rank, waiting for pawnPromote()
	std::atomic<bool> _gameOver{ false };
	bool _wasVsComputer = false;
	bool _lastGameOnline = false; // the current/last game is a Lichess game: rematch goes online
	int  _flaggedKing    = -1;
	bool _resultRecorded = false; // stats recorded for this game (mate can recur after undo)
	bool _resignAfterEngineMove = false; // resign pressed while the robot was thinking
	QString _gameResult;
	QString _resultText;
	QString _gameMode;
	QString _opponentName;
	QString _opponentUserId;
	QString _pendingOffer;
	bool _opponentGone = false;
	int  _claimWinSeconds = -1;
	AppConfig _config;
	int _reviewIndex = -1;

	QVariantList _moveHistoryCache;
	std::vector<std::string> _sanCache; // SAN of shownHistory(), for the PGN
	QDate _gameDate;                    // when the game started; invalid for a saved or continued game
	QVariantMap  _screenshotScene;
};

#endif // CHESSCONNECTOR_H
