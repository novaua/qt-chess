#include "stdafx.h"

#include <QtNetwork>
#include "networkplayer.h"
#include <QDebug>

#define GROUP_ADRESS "239.255.43.32"
#define GROUP_IP    33211

#define TTL 4

NetworkPlayer::NetworkPlayer(const QString &instanceName)
    :_instanceName(instanceName)
{
    _groupAddress = QHostAddress(GROUP_ADRESS);
    _senderSocket = new QUdpSocket(this);

    _senderSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, TTL);

    _receiverSocket = new QUdpSocket(this);
    _receiverSocket->bind(QHostAddress::AnyIPv4, GROUP_IP, QUdpSocket::ShareAddress);
    _receiverSocket->joinMulticastGroup(_groupAddress);
}

void NetworkPlayer::SendAvaliability()
{
    QByteArray datagram = "Avalible:" + QByteArray::fromStdString(_instanceName.toStdString());
    _senderSocket->writeDatagram(datagram.data(), datagram.size(), _groupAddress, GROUP_IP);
}

void NetworkPlayer::ReceiveMessage()
{
    if (_receiverSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_receiverSocket->pendingDatagramSize());
        _receiverSocket->readDatagram(datagram.data(), datagram.size());
        qDebug()<<tr("Received datagram: \"%1\"").arg(datagram.data());
    }
}

void NetworkPlayer::ReceiveMessages()
{
    while (_receiverSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_receiverSocket->pendingDatagramSize());
        _receiverSocket->readDatagram(datagram.data(), datagram.size());
        qDebug()<<tr("Received datagram: \"%1\"").arg(datagram.data());
    }
}
