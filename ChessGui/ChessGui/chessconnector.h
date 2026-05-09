#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QThread>
#include <atomic>
#include <functional>

#include "Game.h"
#include "networkplayer.h"
#include "engineworker.h"

class ChessConnector : public QObject
{
	Q_OBJECT
		Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
		Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
		Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
		Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
		Q_PROPERTY(QStringList PlayersName READ PlayersName NOTIFY PlayersNameChanged)
public:
	explicit ChessConnector(QObject* parent= nullptr);
	~ChessConnector();

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();

	QStringList& PossibleMoves();
	void setPossibleMoves(const QStringList& moves);

	QStringList PlayersName();

signals:
	void boardChanged(int position, const QString& newValue);

	void PossibleMovesChanged();
	void PlayersNameChanged();

	void MoveCountChanged();
	void IsWhiteMoveChanged();
	void IsOnPlayerModeChanged();

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
	void startNewGameWithComputer();

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
	void startEngineThread();
	void stopEngineThread();

private:
	QStringList _possibleMoves;

	Chess::GameAptr _game;
	Chess::HistoryPlayerAptr _player;
	Chess::PawnPromotedCallback _onPawnPromotedCallback;

	NetworkPlayerAptr _netPlayer;

	QThread* _engineThread = nullptr;
	EngineWorker* _engineWorker = nullptr;
	bool _engineThinking = false;
	std::atomic<bool> _gameOver { false };
};

#endif // CHESSCONNECTOR_H
