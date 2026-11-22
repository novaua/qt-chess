#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

class AvatarProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString playerUrl    READ playerUrl    NOTIFY playerAvatarChanged)
    Q_PROPERTY(QString playerName   READ playerName   NOTIFY playerAvatarChanged)
    Q_PROPERTY(QString opponentUrl  READ opponentUrl  NOTIFY opponentAvatarChanged)
    Q_PROPERTY(QString opponentName READ opponentName NOTIFY opponentAvatarChanged)
    // LI-11: what to show when opponentUrl doesn't load (a Lichess player's flair offline): the avatar's own
    // picture (for a Lichess player, online-player.png).
    Q_PROPERTY(QString opponentFallbackUrl READ opponentFallbackUrl NOTIFY opponentAvatarChanged)
    Q_PROPERTY(QStringList names    READ names        CONSTANT) // every avatar, for the pickers

public:
    explicit AvatarProvider(QObject* parent = nullptr);

    static const QStringList& avatarNames();
    QStringList names() const { return avatarNames(); }

    QString playerUrl()      const;
    QString playerName()     const;
    QString playerRawName()  const;
    QString opponentUrl()    const;
    QString opponentName()   const;
    QString opponentRawName() const;
    QString opponentFallbackUrl() const;

    // LI-11: the opponent is a Lichess player: their flair's image (a web URL) when they have one, else the
    // standard avatar online-player.png (a pawn before a globe), never in the picker (avatarNames()).
    void setOpponentOnline(const QString& flairUrl);
    static QString onlinePlayerName() { return QStringLiteral("online-player"); }

public slots:
    void randomize(bool isComputerGame);
    void restore(const QString& playerName, const QString& opponentName, bool isComputerGame);
    Q_INVOKABLE void setPlayerFromUser(const QString& avatarName);
    Q_INVOKABLE void setOpponentFromUser(const QString& avatarName);

signals:
    void playerAvatarChanged();
    void opponentAvatarChanged();

private:
    QString _playerName    = "wizard";
    QString _opponentName  = "unicorn";
    bool    _isComputerGame = false;
    QString _opponentImageUrl; // setOpponentOnline(): a Lichess flair's URL, "" for the avatar's own picture
};
