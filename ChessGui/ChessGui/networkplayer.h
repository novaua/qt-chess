#pragma once
#include <QHostAddress>
#include <memory>

class QUdpSocket;
class NetworkPlayer: public QObject
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
