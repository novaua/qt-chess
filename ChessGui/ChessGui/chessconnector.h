#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <functional>

#include "Game.h"
#include "model.h"

class ChessConnector: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
    Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)
    Q_PROPERTY(int IsWhiteMove READ IsWhiteMove NOTIFY IsWhiteMoveChanged)
    Q_PROPERTY(int IsOnPlayerMode READ IsOnPlayerMode NOTIFY IsOnPlayerModeChanged)
public:
    ChessConnector();
    //Q_INVOKABLE void startNewGame();

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

    void noSavedGame();

public slots:

    void startNewGame();
    void endGame();

    void figureSelected(int index);

    void saveGame();
    bool loadGame();

    void moveNext();
    void movePrev();


private:
    void makeMove(int from, int to);
    void EmitMoveCountUpdates();

private:
    QStringList _possibleMoves;
    std::unique_ptr<Chess::Game> _game;
    Chess::HistoryPlayerAptr _player;
};

#endif // CHESSCONNECTOR_H
