#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QThread>
#include <atomic>
#include <functional>

#include "Game.h"
#include "engineworker.h"

class ChessConnector : public QObject
{
	Q_OBJECT
		Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
		Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
		Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
		Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
		Q_PROPERTY(bool CanContinue READ canContinue NOTIFY canContinueChanged)
		Q_PROPERTY(bool CanLoad READ canLoad NOTIFY canLoadChanged)
public:
	explicit ChessConnector(QObject* parent= nullptr);
	~ChessConnector();

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();
	bool canContinue() const;
	bool canLoad() const;

	QStringList& PossibleMoves();
	void setPossibleMoves(const QStringList& moves);

signals:
	void boardChanged(int position, const QString& newValue);

	void PossibleMovesChanged();

	void MoveCountChanged();
	void IsWhiteMoveChanged();
	void IsOnPlayerModeChanged();
	void canContinueChanged();
	void canLoadChanged();

	void checkNotify();
	void checkMateNotify();

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);

	void noSavedGame();
	void savedOk();
	void noMoreMovesNotify();

	void requestEngineMove();

public slots:
	void startNewGame();
	void startNewGameWithComputer(int level = 3);
	Q_INVOKABLE bool continueGame();

	void endGame();

	void figureSelected(int index);
	void pawnPromote(int index, const QString& piece);

	void saveGame();
	bool loadGame();

	void moveNext();
	void movePrev();

private slots:
	void onEngineMoveComplete();
	void onEngineMoveError(const QString& message);

private:
	void makeMove(int from, int to);
	void EmitMoveCountUpdates();
	void startEngineThread(Chess::EngineLevel level = Chess::EngineLevel{3});
	void stopEngineThread();
	void autoSaveGame(bool isSinglePlayer);
	void deleteAutoSave();
	bool readAutoSaveIsSinglePlayer() const;
	QString getAutoSaveFilePath() const;
	QString getAutoSaveModePath() const;

private:
	QStringList _possibleMoves;

	Chess::GameAptr _game;
	Chess::HistoryPlayerAptr _player;
	Chess::PawnPromotedCallback _onPawnPromotedCallback;

	QThread* _engineThread = nullptr;
	EngineWorker* _engineWorker = nullptr;
	bool _engineThinking = false;
	std::atomic<bool> _gameOver { false };
	int _lastLevel = 3;
};

#endif // CHESSCONNECTOR_H
