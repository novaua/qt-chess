#pragma once
#include <QDate>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include "LichessClient.h"

class LichessAuth;
class QJsonArray;
class QJsonObject;

// Local profiles (users.json): identity, statistics, saves and an optional linked Lichess account.
// Writes are atomic (QSaveFile); on start a readable users.json is copied to backups/ (3 kept).
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
    Q_PROPERTY(QVariantList statistics      READ statistics      NOTIFY statsChanged)
    Q_PROPERTY(QVariantMap  totals          READ totals          NOTIFY statsChanged)
    Q_PROPERTY(QString      statsCreatedDate READ statsCreatedDate NOTIFY statsChanged)
    // Lichess: connected = a token Lichess still accepts; needs reconnect = linked once, but the token
    // was rejected (401) or isn't there (an imported profile).
    Q_PROPERTY(bool         lichessConnected    READ lichessConnected    NOTIFY lichessChanged)
    Q_PROPERTY(bool         lichessNeedsReconnect READ lichessNeedsReconnect NOTIFY lichessChanged)
    Q_PROPERTY(QString      lichessUsername     READ lichessUsername     NOTIFY lichessChanged)
    Q_PROPERTY(QVariantMap  lichessRatings      READ lichessRatings      NOTIFY lichessChanged)
    Q_PROPERTY(bool         lichessLinking      READ lichessLinking      NOTIFY lichessLinkingChanged)
    Q_PROPERTY(bool         hasPendingChallenge READ hasPendingChallenge NOTIFY pendingChallengeChanged)
    Q_PROPERTY(QString      pendingChallengeId  READ pendingChallengeId  NOTIFY pendingChallengeChanged)
    Q_PROPERTY(QString      pendingChallengeUrl READ pendingChallengeUrl NOTIFY pendingChallengeChanged)
    // users.json didn't parse: nothing was loaded and the file is kept until restoreFromBackup() or the next save.
    Q_PROPERTY(bool         dataDamaged      READ dataDamaged      NOTIFY usersChanged)
    Q_PROPERTY(bool         canRestoreBackup READ canRestoreBackup NOTIFY usersChanged)
    // Settings → Data: the backups that can be restored, newest first: { number, date, profiles, current }.
    // current: the same profiles as users.json now.
    Q_PROPERTY(QVariantList backups          READ backups          NOTIFY backupsChanged)

public:
    struct GameSaveInfo {
        bool    isSinglePlayer    = false;
        bool    playerPlaysWhite  = true;
        QString playerAvatarName  = "wizard";
        QString opponentAvatarName = "unicorn";
        QString opponentName;              // "Robot", a profile name or "Guest"; empty in older saves
        QString opponentUserId;            // the opponent's profile in a two-player game; empty = Guest
        QDate   startDate;                 // the day the game started (PGN Date); invalid in older saves
        int     computerLevel  = 0;        // vs Robot: the level played; 0 in older saves
        bool    resultRecorded = false;    // the statistics already have this game (a mate, then undo)
    };

    // One line of the statistics: the games against one opponent.
    struct Record {
        QString opponentName;              // kept so a deleted profile still has a name
        int     games  = 0;                // can be more than wins + draws + losses for older games
        int     wins   = 0;
        int     draws  = 0;
        int     losses = 0;
    };
    // Statistics keys: "computer:<level>" (level 0: before levels were recorded), "user:<id>",
    // "guest", "lichess", "earlier" (two-player games from before results were kept).
    static QString computerKey(int level) { return QStringLiteral("computer:%1").arg(level); }
    static QString userKey(const QString& userId) { return QStringLiteral("user:") + userId; }

    static constexpr int kBackupCount = 3;
    static constexpr int kExportVersion = 1;

    explicit UserManager(QObject* parent = nullptr);

    bool         hasUsers()         const;
    bool         isLoggedIn()       const { return _isLoggedIn; }
    QString      activeUserId()     const { return _activeUserId; }
    QString      activeUserName()   const;
    QString      activeUserAvatar() const;
    int          userCount()        const;
    QVariantList users()            const;
    int          gamesPlayed()          const;
    QVariantList statistics()           const;
    QVariantMap  totals()               const;
    QString      statsCreatedDate()     const;
    bool         lichessConnected()     const;
    bool         lichessNeedsReconnect() const;
    QString      lichessUsername()      const;
    QVariantMap  lichessRatings()       const;
    bool         lichessLinking()       const { return _lichessAuth != nullptr; }
    QString      lichessTokenEncrypted() const;   // C++ only — never expose to QML
    bool         hasPendingChallenge()  const;
    QString      pendingChallengeId()   const;
    QString      pendingChallengeUrl()  const;
    bool         dataDamaged()          const { return _damaged; }
    bool         canRestoreBackup()     const;
    QVariantList backups()              const;

    // outcome: 1 the user won, 0 draw, -1 lost.
    void recordResult(const QString& userId, int outcome, const QString& opponentKey, const QString& opponentName);
    QMap<QString, Record> statsOf(const QString& userId) const;
    QString nameOf(const QString& userId) const;

    GameSaveInfo autoSaveInfo()  const;
    void         setAutoSaveInfo(const GameSaveInfo& info);
    GameSaveInfo savedGameInfo() const;
    void         setSavedGameInfo(const GameSaveInfo& info);
    void         clearAutoSaveInfo();

    void saveLichessCredentials(const QString& plainToken, const QString& username);
    void setLichessAccount(const QString& username, const QVariantMap& ratings); // after /api/account
    void setLichessExpired();                                                     // a 401

    static QString usersFilePath();
    static QString backupFilePath(int n); // backups/users-<n>.json, 1 = newest

public slots:
    Q_INVOKABLE void createUser(const QString& name, const QString& avatarName);
    Q_INVOKABLE void login(const QString& userId);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void updateProfile(const QString& name, const QString& avatarName);
    // Removes the profile, its statistics and its saves. The active profile is logged out first.
    Q_INVOKABLE void deleteUser(const QString& userId);

    // Lichess sign-in. linkLichess / linkLichessToken link the active profile;
    // createUserWithLichess makes a profile named after the account (or logs in to the one already
    // linked to it). Ends with lichessChanged, or lichessLinkFailed; cancelLichessLink ends it silently.
    Q_INVOKABLE void linkLichess();
    Q_INVOKABLE void linkLichessToken(const QString& token);
    Q_INVOKABLE void createUserWithLichess();
    Q_INVOKABLE void cancelLichessLink();
    Q_INVOKABLE void clearLichessToken();

    Q_INVOKABLE void savePendingChallenge(const QString& id, const QString& url);
    Q_INVOKABLE void clearPendingChallenge();

    // Export: one JSON file with the profile, statistics and both saves, never the Lichess token.
    // Returns "" or an error message.
    Q_INVOKABLE QString exportProfile(const QString& userId, const QUrl& file);
    Q_INVOKABLE QString exportFileName(const QString& userId) const; // "<name>.chessprofile"
    // Import, step 1: { error, name, conflictId, conflictName }; conflictId is set when a profile with
    // the same id or name exists.
    Q_INVOKABLE QVariantMap inspectImport(const QUrl& file) const;
    // Step 2: replace = overwrite the conflicting profile; otherwise add it with a new id (and
    // "Name (2)" when the name is taken). Returns "" or an error message.
    Q_INVOKABLE QString importProfile(const QUrl& file, bool replace);

    // The newest backup that parses replaces users.json (the damaged file is kept beside it).
    Q_INVOKABLE bool restoreFromBackup();
    // users.json → backups/users-1.json now (nothing to do when the newest backup is the same).
    Q_INVOKABLE bool backupNow();
    // Backup <n> replaces users.json; the current profiles become the newest backup first. The
    // active profile stays logged in when the backup has it. A profile that exists now keeps its
    // saved-game info (the files aren't in backups) and its Lichess sign-in.
    Q_INVOKABLE bool restoreBackup(int n);

signals:
    void usersChanged();
    void activeUserChanged();
    void statsChanged();
    void lichessChanged();
    void lichessLinkingChanged();
    void lichessLinkFailed(QString error);
    void pendingChallengeChanged();
    void backupsChanged();

private:
    struct User {
        QString      id;
        QString      name;
        QString      avatarName;
        QMap<QString, Record> stats;
        QDate        createdDate;
        QDate        lastPlayed;
        GameSaveInfo autoSave;
        GameSaveInfo savedGame;
        QString      lichessTokenEncrypted;
        QString      lichessUsername;
        bool         lichessExpired = false;
        QVariantMap  lichessRatings;
        QString      pendingChallengeId;
        QString      pendingChallengeUrl;
    };

    QList<User> _users;
    QString     _activeUserId;
    bool        _isLoggedIn = false;
    bool        _damaged    = false;
    LichessAuth* _lichessAuth = nullptr;
    bool         _lichessNewUser = false; // the sign-in creates a profile

    static QJsonObject userToJson(const User& u); // without the Lichess token and pending challenge
    static QList<User> usersFromJson(const QJsonArray& array);
    void load();
    bool loadFrom(const QByteArray& json);
    void save();
    void rotateBackups(const QByteArray& json);
    void emitAllChanged(); // after a restore: every property may have changed
    bool keepDamagedFile(); // false: the damaged users.json couldn't be copied, so it must not be overwritten
    QString uniqueName(const QString& name) const; // "Name", else "Name (2)", "Name (3)", …
    // An export file: its profile (token-free), its saves and the profile here with the same id or
    // name. Returns "" or an error.
    QString readImport(const QUrl& file, User& user, QMap<QString, QByteArray>& files, const User*& conflict) const;
    void startLichessAuth(bool newUser);
    void finishLichessAuth();
    User*       activeUser();
    const User* activeUser() const;
    User*       userById(const QString& id);
    const User* userById(const QString& id) const;
};
