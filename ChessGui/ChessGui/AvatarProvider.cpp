#include "stdafx.h"
#include "AvatarProvider.h"
#include <QRandomGenerator>
#include <QStringList>

static const QStringList& avatarNames()
{
    static const QStringList names = {
        "wizard", "unicorn", "sun",    "flower", "rabbit",
        "mouse",  "girl",    "boy",    "ball",   "star",
        "dragon", "rocket",  "penguin","fox",    "bear",
        "cat",    "ninja",   "pirate", "alien",  "crown"
    };
    return names;
}

AvatarProvider::AvatarProvider(QObject* parent)
    : QObject(parent)
{}

void AvatarProvider::randomize(bool isComputerGame)
{
    const auto& names = avatarNames();
    int idx = (int)QRandomGenerator::global()->bounded((quint32)names.size());
    while (names.size() > 1 && names[idx] == _playerName)
        idx = (int)QRandomGenerator::global()->bounded((quint32)names.size());

    _opponentName   = names[idx];
    _isComputerGame = isComputerGame;

    emit opponentAvatarChanged();
}

static QString capitalize(const QString& s)
{
    if (s.isEmpty()) return s;
    return s[0].toUpper() + s.mid(1);
}

void AvatarProvider::restore(const QString& playerName, const QString& opponentName, bool isComputerGame)
{
    _playerName     = playerName;
    _opponentName   = opponentName;
    _isComputerGame = isComputerGame;
    emit playerAvatarChanged();
    emit opponentAvatarChanged();
}

QString AvatarProvider::playerUrl() const
{
    return "qrc:/app/pics/avatars/" + _playerName + ".png";
}

QString AvatarProvider::playerName() const
{
    return capitalize(_playerName);
}

QString AvatarProvider::playerRawName() const
{
    return _playerName;
}

QString AvatarProvider::opponentUrl() const
{
    return "qrc:/app/pics/avatars/" + (_isComputerGame ? "robot" : _opponentName) + ".png";
}

QString AvatarProvider::opponentName() const
{
    return _isComputerGame ? "Robot" : capitalize(_opponentName);
}

QString AvatarProvider::opponentRawName() const
{
    return _opponentName;
}

void AvatarProvider::setPlayerFromUser(const QString& avatarName)
{
    _playerName = avatarName;
    emit playerAvatarChanged();
}

void AvatarProvider::setOpponentFromUser(const QString& avatarName)
{
    _opponentName   = avatarName;
    _isComputerGame = false;
    emit opponentAvatarChanged();
}
