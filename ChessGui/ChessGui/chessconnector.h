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

public:
    ChessConnector();
    //Q_INVOKABLE void startNewGame();

    int MoveCount();
    int IsWhiteMove();

    QStringList &PossibleMoves();
    void setPossibleMoves(const QStringList &moves);

signals:
    void boardChanged(int position, const QString &newValue);

    void PossibleMovesChanged();

    void MoveCountChanged();
    void IsWhiteMoveChanged();

    void checkNotify();
    void checkMateNotify();

public slots:

    void startNewGame();
    void figureSelected(int index);

    void saveGame();
    void loadGame();

    void moveNext();
    void movePrev();


private:
    void makeMove(int from, int to);

private:
    QStringList _possibleMoves;
    std::unique_ptr<Chess::Game> _game;
};

#endif // CHESSCONNECTOR_H
