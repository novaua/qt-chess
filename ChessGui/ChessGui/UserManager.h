#pragma once
#include <QDate>
#include <QObject>
#include <QString>
#include <QVariantList>
#include "LichessClient.h"

class UserManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool         hasUsers        READ hasUsers        NOTIFY usersChanged)
    Q_PROPERTY(bool         isLoggedIn      READ isLoggedIn      NOTIFY activeUserChanged)
    Q_PROPERTY(QString      activeUserName  READ activeUserName  NOTIFY activeUserChanged)
    Q_PROPERTY(QString      activeUserAvatar READ activeUserAvatar NOTIFY activeUserChanged)
    Q_PROPERTY(int          userCount       READ userCount       NOTIFY usersChanged)
    Q_PROPERTY(QVariantList users           READ users           NOTIFY usersChanged)
    Q_PROPERTY(QString      activeUserId    READ activeUserId    NOTIFY activeUserChanged)
    Q_PROPERTY(int          gamesPlayed     READ gamesPlayed     NOTIFY statsChanged)
    Q_PROPERTY(int          humanWins       READ humanWins       NOTIFY statsChanged)
    Q_PROPERTY(int          computerWins    READ computerWins    NOTIFY statsChanged)
    Q_PROPERTY(QString      statsCreatedDate READ statsCreatedDate NOTIFY statsChanged)
    Q_PROPERTY(bool         lichessConnected    READ lichessConnected    NOTIFY lichessChanged)
    Q_PROPERTY(QString      lichessUsername     READ lichessUsername     NOTIFY lichessChanged)
    Q_PROPERTY(bool         hasPendingChallenge READ hasPendingChallenge NOTIFY pendingChallengeChanged)
    Q_PROPERTY(QString      pendingChallengeId  READ pendingChallengeId  NOTIFY pendingChallengeChanged)
    Q_PROPERTY(QString      pendingChallengeUrl READ pendingChallengeUrl NOTIFY pendingChallengeChanged)

public:
    struct GameSaveInfo {
        bool    isSinglePlayer    = false;
        bool    playerPlaysWhite  = true;
        QString playerAvatarName  = "wizard";
        QString opponentAvatarName = "unicorn";
    };
    explicit UserManager(QObject* parent = nullptr);

    bool         hasUsers()         const;
    bool         isLoggedIn()       const { return _isLoggedIn; }
    QString      activeUserId()     const { return _activeUserId; }
    QString      activeUserName()   const;
    QString      activeUserAvatar() const;
    int          userCount()        const;
    QVariantList users()            const;
    int          gamesPlayed()          const;
    int          humanWins()            const;
    int          computerWins()         const;
    QString      statsCreatedDate()     const;
    bool         lichessConnected()     const;
    QString      lichessUsername()      const;
    QString      lichessTokenEncrypted() const;   // C++ only — never expose to QML
    bool         hasPendingChallenge()  const;
    QString      pendingChallengeId()   const;
    QString      pendingChallengeUrl()  const;

    void recordResult(bool humanWon, bool isComputerGame);

    GameSaveInfo autoSaveInfo()  const;
    void         setAutoSaveInfo(const GameSaveInfo& info);
    GameSaveInfo savedGameInfo() const;
    void         setSavedGameInfo(const GameSaveInfo& info);
    void         clearAutoSaveInfo();

public slots:
    Q_INVOKABLE void createUser(const QString& name, const QString& avatarName);
    Q_INVOKABLE void login(const QString& userId);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void updateProfile(const QString& name, const QString& avatarName);
    Q_INVOKABLE void saveLichessCredentials(const QString& plainToken, const QString& username);
    Q_INVOKABLE void clearLichessToken();
    Q_INVOKABLE void savePendingChallenge(const QString& id, const QString& url);
    Q_INVOKABLE void clearPendingChallenge();

signals:
    void usersChanged();
    void activeUserChanged();
    void statsChanged();
    void lichessChanged();
    void pendingChallengeChanged();

private:
    struct User {
        QString      id;
        QString      name;
        QString      avatarName;
        int          gamesPlayed  = 0;
        int          humanWins    = 0;
        int          computerWins = 0;
        QDate        createdDate;
        GameSaveInfo autoSave;
        GameSaveInfo savedGame;
        QString      lichessTokenEncrypted;
        QString      lichessUsername;
        QString      pendingChallengeId;
        QString      pendingChallengeUrl;
    };

    QList<User> _users;
    QString     _activeUserId;
    bool        _isLoggedIn = false;

    static QString usersFilePath();
    void load();
    void save() const;
    User*       activeUser();
    const User* activeUser() const;
};
