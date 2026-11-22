#include "stdafx.h"
#include "AvatarProvider.h"
#include <QRandomGenerator>
#include <QStringList>

const QStringList& AvatarProvider::avatarNames()
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
    _opponentImageUrl.clear();

    emit opponentAvatarChanged();
}

static QString capitalize(const QString& s)
{
    if (s.isEmpty()) return s;
    return s[0].toUpper() + s.mid(1);
}

void AvatarProvider::restore(const QString& playerName, const QString& opponentName, bool isComputerGame)
{
    // A kept game without them (made outside a game, or an old save): the ones shown now, else the first.
    auto orCurrent = [](const QString& name, const QString& current) {
        return !name.isEmpty() ? name : !current.isEmpty() ? current : avatarNames().first();
    };
    _playerName     = orCurrent(playerName, _playerName);
    _opponentName   = orCurrent(opponentName, _opponentName);
    _isComputerGame = isComputerGame;
    _opponentImageUrl.clear();
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
    return _opponentImageUrl.isEmpty() ? opponentFallbackUrl() : _opponentImageUrl;
}

QString AvatarProvider::opponentFallbackUrl() const
{
    return "qrc:/app/pics/avatars/" + (_isComputerGame ? "robot" : _opponentName) + ".png";
}

QString AvatarProvider::opponentName() const
{
    if (_opponentName == onlinePlayerName())
        return QStringLiteral("Lichess player");
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
    _opponentImageUrl.clear();
    emit opponentAvatarChanged();
}

void AvatarProvider::setOpponentOnline(const QString& flairUrl)
{
    if (_opponentName == onlinePlayerName() && !_isComputerGame && _opponentImageUrl == flairUrl)
        return;
    _opponentName     = onlinePlayerName();
    _isComputerGame   = false;
    _opponentImageUrl = flairUrl;
    emit opponentAvatarChanged();
}
