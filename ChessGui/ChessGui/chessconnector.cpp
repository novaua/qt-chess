#include "stdafx.h"
#include "chessconnector.h"
#include <QDebug>
#include "ChessException.h"
#include <QDir>

using namespace Chess;

const char * EmptyFlag = " ";
const char * DefaltSaveGameFile = "chess.save";

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
    :_game(GameAptr(new Game()))
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
	return _game->IsWhiteMove() ? 1 : 0;
}

void ChessConnector::figureSelected(int index)
{
	if (IsOnPlayerMode()){
		return;
	}

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

void ChessConnector::EmitMoveCountUpdates()
{
	emit MoveCountChanged();
	emit IsWhiteMoveChanged();
}

void ChessConnector::makeMove(int from, int to)
{
	try
	{
		_game->DoMove((BoardPosition)from, (BoardPosition)to);
		EmitMoveCountUpdates();
	}
	catch (ChessException &ex)
	{
		qDebug() << "Exception caught moving [" << from << ", " << to << "]:" << ex.what();
	}
}

void ChessConnector::startNewGame()
{
	_game->Restart();
	EmitMoveCountUpdates();
	qDebug() << "Cpp Game restarted!";
}

QString pathAppend(const QString& path1, const QString& path2)
{
	return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString getSaveGameFilePath()
{
	auto path = pathAppend(QDir::currentPath(), DefaltSaveGameFile);
	return path;
}

bool fileExists(QString path) {
	QFileInfo checkFile(path);

	if (checkFile.exists() && checkFile.isFile()) {
		return true;
	}
	else {
		return false;
	}
}

void ChessConnector::saveGame()
{
	_game->Save(getSaveGameFilePath().toStdString());
	emit savedOk();
}

bool ChessConnector::loadGame()
{
	auto success = false;
	try {
		if (fileExists(getSaveGameFilePath()))
		{
			_game->Load(getSaveGameFilePath().toStdString());
			_player = _game->MakePlayer();
			emit IsOnPlayerModeChanged();
			success = true;
		}
	}
	catch (ChessException &ex)
	{
		qDebug() << "Exception caught while loading game " << ex.what();
	}

	if (!success)
	{
		emit noSavedGame();
	}

	return success;
}

void ChessConnector::moveNext()
{
	_player->MoveNext();
	EmitMoveCountUpdates();
}

void ChessConnector::movePrev()
{
	_player->MoveBack();
	EmitMoveCountUpdates();
}

int ChessConnector::IsOnPlayerMode()
{
	return _player ? 1 : 0;
}

void ChessConnector::endGame()
{
	_game->EndGame();
	_player.reset();
	EmitMoveCountUpdates();
	emit IsOnPlayerModeChanged();
}

ChessConnector::~ChessConnector()
{
    qDebug() << "Game Exited.";
}
