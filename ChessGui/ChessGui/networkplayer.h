#pragma once
#include <QHostAddress>
#include <memory>
#include "Move.h"

enum MessageType
{
    MT_INFO,

    MT_START_NEW_GAME,
    MT_END_GAME,

    MT_MOVE,

    MT_CHAT_MESSAGE,
};

struct INnfoMessage
{
    int Id;
    char Name[256];
};

struct StartNewGameRequest
{
    int Sender;
    int Recepient;

    int SenderColor;
};

struct StartNewGameResponse
{
    int Sender;
    int Recepient;

    int Accepted;
    int WhiteFirst;
};

enum EndGameType
{
    EGT_DEFFERD,
    EGT_DRAW,
};

struct EndGameRequest
{
    int Sender;
    int Recepient;

    EndGameType EndGame;
};

struct EndGameResponse
{
    int Sender;
    int Recepient;

    int Confirmed; //
};

struct MoveRequest
{
    int Sender;
    int Recepient;

    //int SessionId;
    Chess::Move Move;
};

struct MoveResponse
{
    int Sender;
    int Recepient;

    //int SessionId;

    // Zero if move was good
    int MoveError;
};

class QUdpSocket;

class NetworkPlayer : public QObject
{
    Q_OBJECT
public:
    NetworkPlayer(const QString &instanceName);

public:
    void SendAvaliability();
    void ReceiveMessages();

    void ReceiveMessage();

private:

    QString _instanceName;

    QUdpSocket *_senderSocket;
    QUdpSocket *_receiverSocket;

    QHostAddress _groupAddress;
};

typedef std::shared_ptr<NetworkPlayer> NetworkPlayerAptr;
