#include "stdafx.h"
#include "chessconnector.h"
#include "ChessException.h"

#include <QDir>
#include <QDebug>
#include <QHostInfo>

using namespace Chess;

const char *EmptyFlag = " ";
const char *DefaultSaveGameFile = "chess.save";

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
    //,_netPlayer(std::make_shared<NetworkPlayer>( "Vitaly-Nb" /*QHostInfo().hostName() */))
{
    _game->RegisterBoardChanged(
        [&](int index, const Piece &piece)
    {
        emit boardChanged(index, QString::fromStdString(piece.ToString()));
    });

    _game->RegisterGameActionsListeners(
        [&](const EventBase & event)
    {
        if (event.GetType() == EtCheck)
        {
            emit checkNotify();
        }
        else if (event.GetType() == EtCheckMate)
        {
            emit checkMateNotify();
        }
        else if (event.GetType() == EtCastling)
        {
            emit castlingNotify();
        }
        else if (event.GetType() == EtPawnPromotion)
        {
            const PawnPromotionEvent& ppEvent = (PawnPromotionEvent&)event;
            _onPawnPromotedCallback = ppEvent.OnPromoted;
            emit pawnPromotionNotify(ppEvent.GetIndex(), ppEvent.GetColor());
        }
    });

    _game->RegisterLogger(
        [&](const std::string &message)
    {
        qDebug() << "[Game] " << message.c_str();
    });

    ClearBoard(_possibleMoves);

    //_netPlayer->SendAvaliability();
    //_netPlayer->ReceiveMessage();
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
    bool moved = false;
    Move mm;
    if (selected != -1)
    {
        mm = { BoardPosition(selected), BoardPosition(index) };
        makeMove(mm.From, mm.To);
        moved = true;
    }
    else
    {
        for (auto move : _game->GetAllowedMoves(index))
        {
            _possibleMoves[move.To] = QString::number(index);
        }
    }

    emit PossibleMovesChanged();
    if (moved)
    {
        auto result = _UciConnector.MoveTo(mm);
        auto bm = result.second;

        makeMove(bm.From, bm.To);

        emit PossibleMovesChanged();
    }
}

void ChessConnector::pawnPromote(int index, const QString &piece)
{
    auto promotedPiece = Piece::Parse(piece.toStdString());
    _onPawnPromotedCallback({ (BoardPosition)index, promotedPiece });

    qDebug() << "Pawn promoted at " << index << " to " << piece;
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
    auto path = pathAppend(QDir::currentPath(), DefaultSaveGameFile);
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
    if (!_player->CanMove(true)){
        emit noMoreMovesNotify();
    }

	_player->MoveNext();
	EmitMoveCountUpdates();
}

void ChessConnector::movePrev()
{

    if (!_player->CanMove(false)){
        emit noMoreMovesNotify();
    }

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
    EmitMoveCountUpdates();
    emit IsOnPlayerModeChanged();
    ClearBoard(_possibleMoves);
    emit PossibleMovesChanged();
}

ChessConnector::~ChessConnector()
{
    qDebug() << "Game Exited.";
}

QStringList ChessConnector::PlayersName()
{
    //todo
    QStringList players;
    players.append("Joe");
    players.append("Vitaly");

    return players;
}
