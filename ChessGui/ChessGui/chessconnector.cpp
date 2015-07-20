#include "stdafx.h"
#include "chessconnector.h"
#include <QDebug>
#include "ChessException.h"

using namespace Chess;
const char * EmptyFlag = " ";

void ClearBoard(QStringList& board, const QString &cleanValue = EmptyFlag)
{
    auto newBoard = board.empty();
	for (auto i = 0; i < 64; ++i)
	{
        if (newBoard)
            board.append(cleanValue);
        else
            board[i] = cleanValue;
	}
}

ChessConnector::ChessConnector()
    :_game(std::make_unique<Game>())
{
	_game->RegisterBoardChanged(
        [&](int index, const Piece &piece)
	{
        emit boardChanged(index, QString::fromStdString(piece.ToString()));
    });

	ClearBoard(_possibleMoves);
}

int ChessConnector::MoveCount()
{
	return _game->GetMoveCount();
}

int ChessConnector::IsWhiteMove()
{
    return _game->IsWhiteMove()?1:0;
}

void ChessConnector::figureSelected(int index)
{
	//find possible moves for the position and notify IU
    auto pmString = _possibleMoves[index];

    auto selected = pmString == EmptyFlag
		? -1
		: pmString.toInt();

	ClearBoard(_possibleMoves);
    if (selected != -1)
	{
        makeMove(BoardPosition(selected), BoardPosition(index));
	}
	else
	{
        for (auto move : _game->GetAllowedMoves(index))
		{
            _possibleMoves[move.To] = QString::number(index);
		}
	}

	emit PossibleMovesChanged();
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

void ChessConnector::makeMove(int from, int to)
{
    try
    {
        _game->DoMove((BoardPosition)from, (BoardPosition)to);
        emit MoveCountChanged();
        emit IsWhiteMoveChanged();
    }
    catch (ChessException &ex)
    {
        qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
    }
}

void ChessConnector::startNewGame()
{
    qDebug() << "Cpp Starting new game!";
    _game->Restart();
    qDebug() << "Cpp Game restarted!";
}

void ChessConnector::saveGame()
{

}

void ChessConnector::loadGame()
{

}

void ChessConnector::moveNext()
{

}

void ChessConnector::movePrev()
{

}
