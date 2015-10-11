#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <functional>

#include "Game.h"

class ChessConnector : public QObject
{
	Q_OBJECT
		Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
		Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
		Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
		Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
public:
	ChessConnector();
	~ChessConnector();

	int MoveCount();
	int IsWhiteMove();
	int IsOnPlayerMode();

	QStringList &PossibleMoves();
	void setPossibleMoves(const QStringList &moves);

signals:
	void boardChanged(int position, const QString &newValue);

	void PossibleMovesChanged();

	void MoveCountChanged();
	void IsWhiteMoveChanged();
	void IsOnPlayerModeChanged();

	void checkNotify();
	void checkMateNotify();

	void castlingNotify();
	void pawnPromotionNotify(int index, int side);

	void noSavedGame();
	void savedOk();

	public slots:
	void startNewGame();
	void endGame();

	void figureSelected(int index);
	void pawnPromote(int index, const QString &piece);

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
	Chess::PawnPromotedCallback _onPawnPromotedCallback;
};

#endif // CHESSCONNECTOR_H
