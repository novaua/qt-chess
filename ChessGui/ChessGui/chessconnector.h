#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <functional>

#include "Game.h"
#include "networkplayer.h"
#include "ChessEnginePlayer.h"

class ChessConnector : public QObject
{
	Q_OBJECT
		Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
		Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
		Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
		Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
		Q_PROPERTY(QStringList PlayersName READ PlayersName NOTIFY PlayersNameChanged)
public:
	ChessConnector();
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

public slots:
	void startNewGame();
	void startNewGameWithComputer();
	void computerMove();

	void endGame();

	void figureSelected(int index);
	void pawnPromote(int index, const QString& piece);

	void saveGame();
	bool loadGame();

	void moveNext();
	void movePrev();


private:
	void makeMove(int from, int to);
	void EmitMoveCountUpdates();

private:
	QStringList _possibleMoves;

	Chess::GameAptr _game;
	Chess::HistoryPlayerAptr _player;
	Chess::ChessEnginePlayerAptr _chessEnginePlayer;
	Chess::PawnPromotedCallback _onPawnPromotedCallback;

	NetworkPlayerAptr _netPlayer;
};

#endif // CHESSCONNECTOR_H
