#include "../../ChessCore/stdafx.h"
#include "chessconnector.h"
#include <QDebug>

void ClearBoard(QStringList& board, const QString &cleanValue = " ")
{
    board.clear();
    for(auto i=0; i<64; ++i)
    {
        board.append(cleanValue);
    }
}

ChessConnector::ChessConnector()
    :_moveCount(0)
{
    ClearBoard(_board);
    _board[32] = "k";
    _board[40] = "K";
    ClearBoard(_possibleMoves);
    _possibleMoves[38]="39";
    _possibleMoves[44]="45";
    emit PossibleMovesChanged();
}

QString ChessConnector::GetFigureAt(int position)
{
    return _board.at(position);
}

int ChessConnector::MoveCount()
{
    return _moveCount;
}

void ChessConnector::setMoveCount(int moveCount)
{
    _moveCount = moveCount;
    emit MoveCountChanged();
}

void ChessConnector::figureSelected(int position)
{
    //ToDo: find possible moves for the position
    // and notify IU
    // or make move
}

QStringList &ChessConnector::PossibleMoves()
{
    return _possibleMoves;
}

void ChessConnector::setPossibleMoves(const QStringList &moves)
{
    _possibleMoves = moves;
    emit PossibleMovesChanged();
}

QStringList &ChessConnector::ChessBoard()
{
    return _board;
}

void ChessConnector::setChessBoard(const QStringList &board)
{
    _board = board;
    emit ChessBoardChanged();
}

void ChessConnector::makeMove(int index, const QString &figure)
{
    qDebug() << "Make move " << index<<" "<<figure;
}

void ChessConnector::startNewGame()
{
    qDebug() << "Starting new game!";

    emit PossibleMovesChanged();
}
