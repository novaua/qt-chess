#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include "MovesGen.h"
#include <QThread>
#include <array>
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
		Q_PROPERTY(int LastLevel    READ lastLevel    CONSTANT)
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
public:
	explicit ChessConnector(QObject* parent = nullptr);
	~ChessConnector();

	void setAvatarProvider(AvatarProvider* ap) { _avatarProvider = ap; }
	void setUserManager(UserManager* um)       { _userManager = um; }
	void setLichessClient(LichessClient* lc)   { _lichessClient = lc; }

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();
	bool canContinue() const;
	bool canLoad() const;
	int lastLevel()        const { return _config.lastLevel; }
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
	QStringList capturedByDark()  const;
	QStringList capturedByLight() const;

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

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);

	void noSavedGame();
	void savedOk();
	void noMoreMovesNotify();

	void requestEngineMove();

public slots:
	void startNewGame();
	void startNewGameWithComputer(int level = 3);
	Q_INVOKABLE void robotMove();
	Q_INVOKABLE void setGameResult(const QString& result);
	void setUseFen(bool v);
	Q_INVOKABLE void setPlayerPlaysWhite(bool v);
	Q_INVOKABLE void applyMoves(const QString& movesStr);
	Q_INVOKABLE bool continueGame();
	Q_INVOKABLE void startOnlineGame(const QString& gameId, bool playingAsWhite);
	Q_INVOKABLE void resignOnlineGame();

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

private slots:
	void onEngineMoveComplete();
	void onEngineMoveError(const QString& message);

private:
	void makeMove(int from, int to);
	void emitBoardState(int moveIndex);
	void EmitMoveCountUpdates(bool emitHistoryChanged = true);
	void appendMoveToHistory();
	void buildFullHistoryCache();
	void resetMoveHistoryCache();
	void startEngineThread(Chess::EngineLevel level = Chess::EngineLevel{ 3 });
	void stopEngineThread();
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
	std::atomic<bool> _gameOver{ false };
	QString _gameResult;
	AppConfig _config;
	int _reviewIndex = -1;
	std::array<char, 64> _displayedPieces{};

	QVariantList _moveHistoryCache;
};

#endif // CHESSCONNECTOR_H
