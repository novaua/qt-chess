#pragma once
#include <QObject>
#include <QString>

class AvatarProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString playerUrl   READ playerUrl   NOTIFY playerAvatarChanged)
    Q_PROPERTY(QString opponentUrl READ opponentUrl NOTIFY opponentAvatarChanged)

public:
    explicit AvatarProvider(QObject* parent = nullptr);

    QString playerUrl()   const;
    QString opponentUrl() const;

public slots:
    void randomize(bool isComputerGame);

signals:
    void playerAvatarChanged();
    void opponentAvatarChanged();

private:
    QString _playerName    = "wizard";
    QString _opponentName  = "unicorn";
    bool    _isComputerGame = false;
};
