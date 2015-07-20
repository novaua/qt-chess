#include "stdafx.h"
#include "chessconnector.h"
#include <QDebug>
#include "ChessException.h"

using namespace Chess;
const char * EmptyFlag = " ";

void ClearBoard(QStringList& board, const QString &cleanValue = EmptyFlag)
{
	board.clear();
	for (auto i = 0; i < 64; ++i)
	{
		board.append(cleanValue);
	}
}

ChessConnector::ChessConnector()
	:_moveCount(0),
	_game(std::make_unique<Game>())
{
	_game->RegisterBoardChanged(
		[&](const Move &move)
	{
		_board[move.From] = QString::fromStdString(_game->GetPieceAt(move.From).ToString());
		_board[move.To] = QString::fromStdString(_game->GetPieceAt(move.To).ToString());
		emit ChessBoardChanged();
	});

	ClearBoard(_board);
	ClearBoard(_possibleMoves);
}

QString ChessConnector::GetFigureAt(int position)
{
	return _board.at(position);
}

int ChessConnector::MoveCount()
{
	return _game->GetMoveCount();
}

void ChessConnector::figureSelected(int position)
{
	//find possible moves for the position and notify IU
	auto pmString = _possibleMoves[position];
	auto pmInt = pmString == EmptyFlag
		? -1
		: pmString.toInt();
	ClearBoard(_possibleMoves);
	if (pmInt != -1)
	{
		try
		{
			// or make move
			_game->DoMove((BoardPosition)pmInt, (BoardPosition)position);
			emit MoveCountChanged();
		}
		catch (ChessException &ex)
		{
			qDebug() << "Exception caught moving [" << pmInt << ", " << position << "]:" << ex.what();
			//okay do nothing
		}
	}
	else
	{
		for (auto move : _game->GetAllowedMoves(position))
		{
			_possibleMoves[move.To] = QString::number(position);
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
	qDebug() << "Make move " << index << " " << figure;
}

void ChessConnector::startNewGame()
{
	qDebug() << "Starting new game!";
	if (_game->GetMoveCount() != 0) {
		_game->Restart();
	}

	for (auto i = 0u; i < 64u; ++i){
		_board[i] = QString::fromStdString(_game->GetPieceAt(i).ToString());
	}

	emit ChessBoardChanged();
}
