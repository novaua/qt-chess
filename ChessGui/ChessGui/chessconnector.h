#ifndef CHESSCONNECTOR_H
#define CHESSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <functional>

#include "Game.h"

class ChessConnector: public QObject
{
    Q_OBJECT

    Q_PROPERTY(int MoveCount READ MoveCount NOTIFY MoveCountChanged)
    Q_PROPERTY(QStringList PossibleMoves READ PossibleMoves WRITE setPossibleMoves NOTIFY PossibleMovesChanged)

    Q_PROPERTY(QStringList ChessBoard READ ChessBoard WRITE setChessBoard NOTIFY ChessBoardChanged)

public:
    ChessConnector();

    Q_INVOKABLE void startNewGame();
    Q_INVOKABLE QString GetFigureAt(int position);
    Q_INVOKABLE void makeMove(int index, const QString &figure);

    int MoveCount();
    void setMoveCount(int moveCount);

    QStringList &PossibleMoves();
    void setPossibleMoves(const QStringList &moves);

    QStringList &ChessBoard();
    void setChessBoard(const QStringList &board);

signals:
    //void boardChanged(int position, const QString &newValue);

    void MoveCountChanged();
    void PossibleMovesChanged();
    void ChessBoardChanged();

    void checkNotify();
    void checkMateNotify();

    // Likely we do not need it
    //void doQmlMove(QString move);

public slots:
    void figureSelected(int index);

private:
    QStringList _board;
    QStringList _possibleMoves;

    int _moveCount;

    int _selectedPosition;
    std::unique_ptr<Chess::Game> _game;
};

#endif // CHESSCONNECTOR_H
