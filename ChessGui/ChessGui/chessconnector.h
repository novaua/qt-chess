#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
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
#include "LichessAuth.h"

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
		Q_PROPERTY(QStringList CapturedByDark  READ capturedByDark  NOTIFY capturedChanged)
		Q_PROPERTY(QStringList CapturedByLight READ capturedByLight NOTIFY capturedChanged)
		Q_PROPERTY(bool EngineThinking    READ engineThinking    NOTIFY engineThinkingChanged)
		Q_PROPERTY(bool UseFen            READ useFen            WRITE setUseFen NOTIFY useFenChanged)
		Q_PROPERTY(bool PlayerPlaysWhite  READ playerPlaysWhite  NOTIFY playerPlaysWhiteChanged)
		Q_PROPERTY(QVariantList MoveHistory READ moveHistory NOTIFY moveHistoryChanged)
		Q_PROPERTY(QString      GameResult  READ gameResult  NOTIFY gameResultChanged)
		Q_PROPERTY(bool ReviewMode    READ reviewMode    NOTIFY reviewStateChanged)
		Q_PROPERTY(int  ReviewIndex   READ reviewIndex   NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewPrev READ canReviewPrev NOTIFY reviewStateChanged)
		Q_PROPERTY(bool CanReviewNext READ canReviewNext NOTIFY reviewStateChanged)
		Q_PROPERTY(bool IsOnlineGame  READ isOnlineGame  NOTIFY isOnlineGameChanged)
		Q_PROPERTY(bool CanRematch    READ canRematch    NOTIFY canRematchChanged)
		Q_PROPERTY(int  FlaggedKing   READ flaggedKing   NOTIFY flaggedKingChanged) // square of a king that resigned / lost on time, or -1
		// --screenshot mode: { scene, moves, black, dark, width, height }; empty in a normal run
		Q_PROPERTY(QVariantMap ScreenshotScene READ screenshotScene CONSTANT)
public:
	explicit ChessConnector(QObject* parent = nullptr);
	~ChessConnector();

	void setAvatarProvider(AvatarProvider* ap) { _avatarProvider = ap; }
	void setUserManager(UserManager* um)       { _userManager = um; }
	void setLichessClient(LichessClient* lc);
	void setScreenshotScene(const QVariantMap& scene) { _screenshotScene = scene; }

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();
	bool canContinue() const;
	bool canLoad() const;
	int lastLevel()        const { return _config.lastLevel; }
	QVariantList levels()  const; // computer levels 1-5: { level, name, elo, eloEstimated, hint }
	int  lastMoveFrom()   const;
	int  lastMoveTo()     const;
	bool engineThinking()    const { return _engineThinking; }
	bool useFen()            const { return _config.useFen; }
	bool playerPlaysWhite()  const { return _config.playerPlaysWhite; }
	QVariantList moveHistory() const;
	QString      gameResult()  const { return _gameResult; }
	bool reviewMode()    const { return _reviewIndex >= 0; }
	int  reviewIndex()   const { return _player ? _player->GetPointer() : _reviewIndex; }
	bool canReviewPrev() const;
	bool canReviewNext() const;
	bool isOnlineGame()  const { return !_onlineGameId.isEmpty(); }
	// Local games can always be replayed; a Lichess game needs its settings and a token.
	bool canRematch()    const { return !_lastGameOnline || (_lichessClient && _lichessClient->canRematch()); }
	// Hidden while reviewing earlier moves: the king may not be on that square then.
	int  flaggedKing()   const { return reviewMode() ? -1 : _flaggedKing; }
	QStringList capturedByDark()  const;
	QStringList capturedByLight() const;
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
	void drawOfferReceived();
	void takebackRequested();
	void lichessLoginResult(bool ok, QString username);

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);

	void noSavedGame();
	void savedOk();
	void noMoreMovesNotify();

	void requestEngineMove();

	void screenshotSceneReady(); // QML has set up the --screenshot scene: grab the window

public slots:
	void startNewGame();
	void startNewGameWithComputer(int level = 3);
	Q_INVOKABLE void robotMove();
	Q_INVOKABLE void setGameResult(const QString& result); // non-empty: the game is over
	Q_INVOKABLE void flagKing(bool white); // mark that side's king as the loser (resign / time)
	void setUseFen(bool v);
	Q_INVOKABLE void setPlayerPlaysWhite(bool v);
	Q_INVOKABLE void applyMoves(const QString& movesStr);
	Q_INVOKABLE bool continueGame();
	Q_INVOKABLE void startOnlineGame(const QString& gameId, bool playingAsWhite);

	void endGame();

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

	Q_INVOKABLE void loginWithLichess();

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

private:
	void makeMove(int from, int to);
	void onHumanMoveDone();
	void emitBoardState(int moveIndex);
	void EmitMoveCountUpdates(bool emitHistoryChanged = true);
	void appendMoveToHistory();
	void buildFullHistoryCache();
	void resetMoveHistoryCache();
	void startEngineThread(Chess::EngineLevel level = Chess::EngineLevel{ 3 });
	void stopEngineThread();
	void beginGame(bool online); // resets per-game end state; online: rematch goes to Lichess
	bool isMateBy(bool white) const;
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
	AppConfig _config;
	int _reviewIndex = -1;
	LichessAuth* _lichessAuth = nullptr;

	QVariantList _moveHistoryCache;
	QVariantMap  _screenshotScene;
};

#endif // CHESSCONNECTOR_H
