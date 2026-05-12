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
    int idx  = (int)QRandomGenerator::global()->bounded((quint32)names.size());
    int idx2 = (idx + 1 + (int)QRandomGenerator::global()->bounded((quint32)(names.size() - 1))) % names.size();

    _playerName     = names[idx];
    _opponentName   = names[idx2];
    _isComputerGame = isComputerGame;

    emit playerAvatarChanged();
    emit opponentAvatarChanged();
}

QString AvatarProvider::playerUrl() const
{
    return "qrc:/app/pics/avatars/" + _playerName + ".png";
}

QString AvatarProvider::opponentUrl() const
{
    return "qrc:/app/pics/avatars/" + (_isComputerGame ? "robot" : _opponentName) + ".png";
}
