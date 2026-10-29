#include "stdafx.h"
#include "UserManager.h"
#include "AppConfig.h"
#include "AvatarProvider.h"
#include "LichessAuth.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>

static const QString kExportFormat = QStringLiteral("chessplusplus-profile");
// The per-user files an export carries (AppConfig::autoSaveFilePath / savedGameFilePath).
static const QStringList kProfileFiles = { QStringLiteral("chess.autosave"), QStringLiteral("chess.save") };

UserManager::UserManager(QObject* parent)
    : QObject(parent)
{
    load();
}

QString UserManager::usersFilePath()
{
    return AppConfig::appDataDir() + "/users.json";
}

QString UserManager::backupFilePath(int n)
{
    return AppConfig::appDataDir() + QStringLiteral("/backups/users-%1.json").arg(n);
}

// Without creating it (AppConfig's save paths do).
static QString userDir(const QString& userId)
{
    return AppConfig::appDataDir() + "/users/" + userId;
}

static bool writeFileAtomically(const QString& path, const QByteArray& data, QString* error = nullptr)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit())
        return true;
    qWarning() << "UserManager: can't write" << path << file.errorString();
    if (error) *error = file.errorString();
    return false;
}

static QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

static QJsonObject parseObject(const QByteArray& json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    return err.error == QJsonParseError::NoError && doc.isObject() ? doc.object() : QJsonObject();
}

// "Today", "Yesterday", "Oct 12", "Oct 12, 2025"; "" = never.
static QString dayText(const QDate& day)
{
    if (!day.isValid()) return {};
    const QDate today = QDate::currentDate();
    if (day == today)             return QStringLiteral("Today");
    if (day == today.addDays(-1)) return QStringLiteral("Yesterday");
    return day.toString(day.year() == today.year() ? QStringLiteral("MMM d") : QStringLiteral("MMM d, yyyy"));
}

bool UserManager::hasUsers()    const { return !_users.isEmpty(); }
int  UserManager::userCount()   const { return _users.size(); }

QString UserManager::activeUserName() const
{
    const auto* u = activeUser();
    return u ? u->name : QString();
}

QString UserManager::activeUserAvatar() const
{
    const auto* u = activeUser();
    return u ? u->avatarName : QStringLiteral("wizard");
}

QString UserManager::nameOf(const QString& userId) const
{
    const auto* u = userById(userId);
    return u ? u->name : QString();
}

QVariantList UserManager::users() const
{
    QVariantList result;
    for (const auto& u : _users) {
        QVariantMap m;
        m[QStringLiteral("id")]        = u.id;
        m[QStringLiteral("name")]      = u.name;
        m[QStringLiteral("avatarName")] = u.avatarName;
        m[QStringLiteral("avatarUrl")]  = "qrc:/app/pics/avatars/" + u.avatarName + ".png";
        m[QStringLiteral("lichessUsername")] = u.lichessUsername;
        m[QStringLiteral("lastPlayed")]      = dayText(u.lastPlayed);
        result << m;
    }
    return result;
}

static UserManager::Record sumOf(const QMap<QString, UserManager::Record>& stats)
{
    UserManager::Record total;
    for (const auto& r : stats) {
        total.games  += r.games;
        total.wins   += r.wins;
        total.draws  += r.draws;
        total.losses += r.losses;
    }
    return total;
}

int UserManager::gamesPlayed() const
{
    const auto* u = activeUser();
    return u ? sumOf(u->stats).games : 0;
}

QVariantMap UserManager::totals() const
{
    const auto* u = activeUser();
    const Record t = u ? sumOf(u->stats) : Record{};
    return { { QStringLiteral("games"), t.games }, { QStringLiteral("wins"), t.wins },
             { QStringLiteral("draws"), t.draws }, { QStringLiteral("losses"), t.losses } };
}

// The active profile's records, in display order: the robot by level, people by name, Guest,
// Lichess, older games. Each: { kind, level, name, games, wins, draws, losses }.
QVariantList UserManager::statistics() const
{
    const auto* u = activeUser();
    if (!u) return {};

    static const QStringList kindOrder = { "computer", "user", "guest", "lichess", "earlier" };
    struct Row { int kindIndex; int level; QString name; QVariantMap map; };
    QList<Row> rows;
    for (auto it = u->stats.cbegin(); it != u->stats.cend(); ++it) {
        const QString kind = it.key().section(':', 0, 0);
        const QString arg  = it.key().section(':', 1);
        const int level = kind == QLatin1String("computer") ? arg.toInt() : 0;
        QString name = it->opponentName;
        if (kind == QLatin1String("user") && !nameOf(arg).isEmpty())
            name = nameOf(arg); // renamed since
        const Record& r = *it;
        rows.append({ (int)kindOrder.indexOf(kind), level, name, {
            { QStringLiteral("kind"), kind }, { QStringLiteral("level"), level },
            { QStringLiteral("name"), name }, { QStringLiteral("games"), r.games },
            { QStringLiteral("wins"), r.wins }, { QStringLiteral("draws"), r.draws },
            { QStringLiteral("losses"), r.losses } } });
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        if (a.kindIndex != b.kindIndex) return a.kindIndex < b.kindIndex;
        if (a.level != b.level) return a.level < b.level;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    QVariantList result;
    for (const auto& row : rows)
        result << row.map;
    return result;
}

QMap<QString, UserManager::Record> UserManager::statsOf(const QString& userId) const
{
    const auto* u = userById(userId);
    return u ? u->stats : QMap<QString, Record>{};
}

bool    UserManager::lichessConnected()     const { const auto* u = activeUser(); return u && !u->lichessTokenEncrypted.isEmpty() && !u->lichessExpired; }
bool    UserManager::lichessNeedsReconnect() const { const auto* u = activeUser(); return u && !u->lichessUsername.isEmpty() && !lichessConnected(); }
QString UserManager::lichessUsername()      const { const auto* u = activeUser(); return u ? u->lichessUsername       : QString(); }
QVariantMap UserManager::lichessRatings()   const { const auto* u = activeUser(); return u ? u->lichessRatings        : QVariantMap(); }
QString UserManager::lichessTokenEncrypted() const { const auto* u = activeUser(); return u ? u->lichessTokenEncrypted : QString(); }
bool    UserManager::hasPendingChallenge()  const { const auto* u = activeUser(); return u && !u->pendingChallengeId.isEmpty(); }
QString UserManager::pendingChallengeId()   const { const auto* u = activeUser(); return u ? u->pendingChallengeId   : QString(); }
QString UserManager::pendingChallengeUrl()  const { const auto* u = activeUser(); return u ? u->pendingChallengeUrl  : QString(); }

QString UserManager::statsCreatedDate() const
{
    const auto* u = activeUser();
    return u ? u->createdDate.toString("MMMM d, yyyy") : QString();
}

void UserManager::createUser(const QString& name, const QString& avatarName)
{
    User u;
    u.id          = QUuid::createUuid().toString(QUuid::WithoutBraces);
    u.name        = name.trimmed();
    u.avatarName  = avatarName;
    u.createdDate = QDate::currentDate();
    _users.append(u);
    save();
    emit usersChanged();
    login(u.id);
}

void UserManager::login(const QString& userId)
{
    if (!userById(userId)) return;
    _activeUserId = userId;
    _isLoggedIn   = true;
    save();
    emit activeUserChanged();
    emit statsChanged();
    emit lichessChanged();
    emit pendingChallengeChanged();
}

void UserManager::logout()
{
    cancelLichessLink();
    _isLoggedIn = false;
    // _activeUserId kept so the profile list can highlight the last user
    emit activeUserChanged();
    emit statsChanged();
}

void UserManager::updateProfile(const QString& name, const QString& avatarName)
{
    auto* u = activeUser();
    if (!u) return;
    u->name       = name.trimmed();
    u->avatarName = avatarName;
    save();
    emit activeUserChanged();
    emit usersChanged();
}

void UserManager::deleteUser(const QString& userId)
{
    const auto it = std::find_if(_users.begin(), _users.end(), [&](const User& u) { return u.id == userId; });
    if (it == _users.end()) return;
    const bool wasActive = userId == _activeUserId;
    if (wasActive) {
        cancelLichessLink();
        _isLoggedIn = false;
        _activeUserId.clear();
    }
    _users.erase(it);
    QDir(userDir(userId)).removeRecursively();
    save();
    emit usersChanged();
    if (wasActive) {
        emit activeUserChanged();
        emit statsChanged();
        emit lichessChanged();
        emit pendingChallengeChanged();
    }
}

UserManager::GameSaveInfo UserManager::autoSaveInfo() const
{
    const auto* u = activeUser();
    return u ? u->autoSave : GameSaveInfo{};
}

void UserManager::setAutoSaveInfo(const GameSaveInfo& info)
{
    auto* u = activeUser();
    if (!u) return;
    u->autoSave   = info;
    u->lastPlayed = QDate::currentDate();
    save();
    emit usersChanged(); // last played
}

UserManager::GameSaveInfo UserManager::savedGameInfo() const
{
    const auto* u = activeUser();
    return u ? u->savedGame : GameSaveInfo{};
}

void UserManager::setSavedGameInfo(const GameSaveInfo& info)
{
    auto* u = activeUser();
    if (!u) return;
    u->savedGame = info;
    save();
}

void UserManager::saveLichessCredentials(const QString& plainToken, const QString& username)
{
    auto* u = activeUser();
    if (!u) return;
    if (u->lichessUsername.compare(username, Qt::CaseInsensitive) != 0)
        u->lichessRatings.clear(); // another account's ratings
    u->lichessTokenEncrypted = LichessClient::encryptToken(plainToken);
    u->lichessUsername       = username;
    u->lichessExpired        = false;
    save();
    emit lichessChanged();
    emit usersChanged(); // the Lichess badge
}

void UserManager::setLichessAccount(const QString& username, const QVariantMap& ratings)
{
    auto* u = activeUser();
    if (!u || u->lichessUsername.compare(username, Qt::CaseInsensitive) != 0) return;
    if (u->lichessRatings == ratings && u->lichessUsername == username) return;
    u->lichessUsername = username; // Lichess's spelling of the name
    u->lichessRatings  = ratings;
    save();
    emit lichessChanged();
}

void UserManager::setLichessExpired()
{
    auto* u = activeUser();
    if (!u || u->lichessTokenEncrypted.isEmpty() || u->lichessExpired) return;
    u->lichessExpired = true;
    save();
    emit lichessChanged();
}

void UserManager::clearLichessToken()
{
    auto* u = activeUser();
    if (!u) return;
    u->lichessTokenEncrypted.clear();
    u->lichessUsername.clear();
    u->lichessRatings.clear();
    u->lichessExpired = false;
    save();
    emit lichessChanged();
    emit usersChanged();
}

void UserManager::startLichessAuth(bool newUser)
{
    cancelLichessLink();
    _lichessNewUser = newUser;
    _lichessAuth = new LichessAuth(this);

    connect(_lichessAuth, &LichessAuth::loginSucceeded, this, [this](const QString& token, const QString& username) {
        const bool newUser = _lichessNewUser;
        finishLichessAuth();
        if (newUser) {
            // Signed in with an account a profile is already linked to: that profile.
            const auto it = std::find_if(_users.cbegin(), _users.cend(), [&](const User& u) {
                return u.lichessUsername.compare(username, Qt::CaseInsensitive) == 0;
            });
            if (it != _users.cend()) {
                login(it->id);
            } else {
                const auto& avatars = AvatarProvider::avatarNames();
                createUser(username, avatars[QRandomGenerator::global()->bounded((int)avatars.size())]);
            }
        }
        saveLichessCredentials(token, username);
    });
    connect(_lichessAuth, &LichessAuth::loginFailed, this, [this](const QString& error) {
        qDebug() << "[LichessAuth] sign-in failed:" << error;
        finishLichessAuth();
        emit lichessLinkFailed(error);
    });
    emit lichessLinkingChanged();
}

void UserManager::finishLichessAuth()
{
    if (!_lichessAuth) return;
    _lichessAuth->cancel();
    _lichessAuth->deleteLater();
    _lichessAuth = nullptr;
    emit lichessLinkingChanged();
}

void UserManager::linkLichess()
{
    if (!activeUser()) return;
    startLichessAuth(false);
    _lichessAuth->startLogin();
}

void UserManager::linkLichessToken(const QString& token)
{
    if (!activeUser() || token.trimmed().isEmpty()) return;
    startLichessAuth(false);
    _lichessAuth->useToken(token.trimmed());
}

void UserManager::createUserWithLichess()
{
    startLichessAuth(true);
    _lichessAuth->startLogin();
}

void UserManager::cancelLichessLink()
{
    finishLichessAuth();
}

void UserManager::savePendingChallenge(const QString& id, const QString& url)
{
    auto* u = activeUser();
    if (!u) return;
    u->pendingChallengeId  = id;
    u->pendingChallengeUrl = url;
    save();
    emit pendingChallengeChanged();
}

void UserManager::clearPendingChallenge()
{
    auto* u = activeUser();
    if (!u) return;
    u->pendingChallengeId.clear();
    u->pendingChallengeUrl.clear();
    save();
    emit pendingChallengeChanged();
}

void UserManager::clearAutoSaveInfo()
{
    auto* u = activeUser();
    if (!u) return;
    u->autoSave = GameSaveInfo{};
    save();
}

void UserManager::recordResult(const QString& userId, int outcome, const QString& opponentKey, const QString& opponentName)
{
    auto* u = userById(userId);
    if (!u) return;
    Record& r = u->stats[opponentKey];
    r.opponentName = opponentName;
    r.games++;
    if (outcome > 0)       r.wins++;
    else if (outcome == 0) r.draws++;
    else                   r.losses++;
    u->lastPlayed = QDate::currentDate();
    save();
    emit usersChanged();
    if (userId == _activeUserId)
        emit statsChanged();
}

UserManager::User* UserManager::activeUser()
{
    return userById(_activeUserId);
}

const UserManager::User* UserManager::activeUser() const
{
    return userById(_activeUserId);
}

UserManager::User* UserManager::userById(const QString& id)
{
    for (auto& u : _users)
        if (u.id == id) return &u;
    return nullptr;
}

const UserManager::User* UserManager::userById(const QString& id) const
{
    for (const auto& u : _users)
        if (u.id == id) return &u;
    return nullptr;
}

// ── users.json ────────────────────────────────────────────────────────────

static QJsonObject saveInfoToJson(const UserManager::GameSaveInfo& info)
{
    QJsonObject o;
    o[QStringLiteral("isSinglePlayer")]   = info.isSinglePlayer;
    o[QStringLiteral("playerPlaysWhite")] = info.playerPlaysWhite;
    o[QStringLiteral("playerAvatarName")]  = info.playerAvatarName;
    o[QStringLiteral("opponentAvatarName")] = info.opponentAvatarName;
    o[QStringLiteral("opponentName")]       = info.opponentName;
    if (!info.opponentUserId.isEmpty())
        o[QStringLiteral("opponentUserId")] = info.opponentUserId;
    if (info.startDate.isValid())
        o[QStringLiteral("startDate")]      = info.startDate.toString(Qt::ISODate);
    return o;
}

static UserManager::GameSaveInfo saveInfoFromJson(const QJsonObject& o)
{
    UserManager::GameSaveInfo info;
    info.isSinglePlayer    = o.value(QStringLiteral("isSinglePlayer")).toBool(false);
    info.playerPlaysWhite  = o.value(QStringLiteral("playerPlaysWhite")).toBool(true);
    info.playerAvatarName  = o.value(QStringLiteral("playerAvatarName")).toString(QStringLiteral("wizard"));
    info.opponentAvatarName = o.value(QStringLiteral("opponentAvatarName")).toString(QStringLiteral("unicorn"));
    info.opponentName       = o.value(QStringLiteral("opponentName")).toString();
    info.opponentUserId     = o.value(QStringLiteral("opponentUserId")).toString();
    info.startDate          = QDate::fromString(o.value(QStringLiteral("startDate")).toString(), Qt::ISODate);
    return info;
}

static QJsonObject statsToJson(const QMap<QString, UserManager::Record>& stats)
{
    QJsonObject byOpponent;
    for (auto it = stats.cbegin(); it != stats.cend(); ++it) {
        byOpponent[it.key()] = QJsonObject{
            { QStringLiteral("name"),   it->opponentName },
            { QStringLiteral("games"),  it->games },
            { QStringLiteral("wins"),   it->wins },
            { QStringLiteral("draws"),  it->draws },
            { QStringLiteral("losses"), it->losses },
        };
    }
    return { { QStringLiteral("byOpponent"), byOpponent } };
}

static QMap<QString, UserManager::Record> statsFromJson(const QJsonObject& stats)
{
    QMap<QString, UserManager::Record> result;
    if (!stats.contains(QStringLiteral("byOpponent"))) {
        // Version 1 kept totals only: games, and wins/losses against the robot (level unknown).
        const int games        = stats.value(QStringLiteral("gamesPlayed")).toInt(0);
        const int humanWins    = stats.value(QStringLiteral("humanWins")).toInt(0);
        const int computerWins = stats.value(QStringLiteral("computerWins")).toInt(0);
        if (humanWins + computerWins > 0)
            result[UserManager::computerKey(0)] = { QStringLiteral("Robot"), humanWins + computerWins, humanWins, 0, computerWins };
        if (games > humanWins + computerWins)
            result[QStringLiteral("earlier")] = { QString(), games - humanWins - computerWins, 0, 0, 0 };
        return result;
    }
    const QJsonObject byOpponent = stats.value(QStringLiteral("byOpponent")).toObject();
    for (auto it = byOpponent.constBegin(); it != byOpponent.constEnd(); ++it) {
        const QJsonObject o = it.value().toObject();
        result[it.key()] = { o.value(QStringLiteral("name")).toString(), o.value(QStringLiteral("games")).toInt(),
                             o.value(QStringLiteral("wins")).toInt(),    o.value(QStringLiteral("draws")).toInt(),
                             o.value(QStringLiteral("losses")).toInt() };
    }
    return result;
}

QJsonObject UserManager::userToJson(const User& u)
{
    QJsonObject obj;
    obj[QStringLiteral("id")]          = u.id;
    obj[QStringLiteral("name")]        = u.name;
    obj[QStringLiteral("avatarName")]  = u.avatarName;
    obj[QStringLiteral("createdDate")] = u.createdDate.toString(Qt::ISODate);
    if (u.lastPlayed.isValid())
        obj[QStringLiteral("lastPlayed")] = u.lastPlayed.toString(Qt::ISODate);
    obj[QStringLiteral("stats")]       = statsToJson(u.stats);
    obj[QStringLiteral("autoSave")]    = saveInfoToJson(u.autoSave);
    obj[QStringLiteral("savedGame")]   = saveInfoToJson(u.savedGame);
    obj[QStringLiteral("lichessUsername")] = u.lichessUsername;
    if (!u.lichessRatings.isEmpty())
        obj[QStringLiteral("lichessRatings")] = QJsonObject::fromVariantMap(u.lichessRatings);
    return obj;
}

QList<UserManager::User> UserManager::usersFromJson(const QJsonArray& array)
{
    QList<User> users;
    for (const auto& val : array) {
        const QJsonObject obj = val.toObject();
        User u;
        u.id         = obj.value(QStringLiteral("id")).toString();
        u.name       = obj.value(QStringLiteral("name")).toString();
        u.avatarName = obj.value(QStringLiteral("avatarName")).toString(QStringLiteral("wizard"));
        u.createdDate = QDate::fromString(obj.value(QStringLiteral("createdDate")).toString(), Qt::ISODate);
        if (!u.createdDate.isValid()) u.createdDate = QDate::currentDate();
        u.lastPlayed  = QDate::fromString(obj.value(QStringLiteral("lastPlayed")).toString(), Qt::ISODate);
        u.stats       = statsFromJson(obj.value(QStringLiteral("stats")).toObject());
        u.autoSave              = saveInfoFromJson(obj.value(QStringLiteral("autoSave")).toObject());
        u.savedGame             = saveInfoFromJson(obj.value(QStringLiteral("savedGame")).toObject());
        u.lichessTokenEncrypted = obj.value(QStringLiteral("lichessToken")).toString();
        u.lichessUsername       = obj.value(QStringLiteral("lichessUsername")).toString();
        u.lichessExpired        = obj.value(QStringLiteral("lichessExpired")).toBool(false);
        u.lichessRatings        = obj.value(QStringLiteral("lichessRatings")).toObject().toVariantMap();
        u.pendingChallengeId    = obj.value(QStringLiteral("pendingChallengeId")).toString();
        u.pendingChallengeUrl   = obj.value(QStringLiteral("pendingChallengeUrl")).toString();
        if (!u.id.isEmpty() && !u.name.isEmpty())
            users.append(u);
    }
    return users;
}

bool UserManager::loadFrom(const QByteArray& json)
{
    const QJsonObject root = parseObject(json);
    if (root.isEmpty() || !root.value(QStringLiteral("users")).isArray())
        return false;

    _users = usersFromJson(root.value(QStringLiteral("users")).toArray());
    _activeUserId = root.value(QStringLiteral("activeUserId")).toString();
    // One profile: straight in. Several: the profile list asks who is playing.
    _isLoggedIn = userById(_activeUserId) && _users.size() == 1;
    return true;
}

void UserManager::load()
{
    if (!QFile::exists(usersFilePath()))
        return; // first run
    const QByteArray json = readFile(usersFilePath());
    if (!loadFrom(json)) {
        qWarning() << "UserManager: users.json can't be read; it is kept until a restore or the next save";
        _damaged = true;
        return;
    }
    rotateBackups(json);
}

// backups/users-1.json is the newest. An unchanged file isn't copied again, so the older copies stay.
void UserManager::rotateBackups(const QByteArray& json) const
{
    if (readFile(backupFilePath(1)) == json)
        return;
    QFile::remove(backupFilePath(kBackupCount));
    for (int n = kBackupCount - 1; n >= 1; --n)
        QFile::rename(backupFilePath(n), backupFilePath(n + 1));
    writeFileAtomically(backupFilePath(1), json);
}

bool UserManager::canRestoreBackup() const
{
    if (!_damaged) return false;
    for (int n = 1; n <= kBackupCount; ++n)
        if (!parseObject(readFile(backupFilePath(n))).isEmpty())
            return true;
    return false;
}

// users.damaged-<time>.json: never overwrite a file the user may still want to recover.
void UserManager::keepDamagedFile()
{
    if (!_damaged) return;
    _damaged = false;
    const QString target = AppConfig::appDataDir() + "/users.damaged-"
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss")) + ".json";
    if (!QFile::rename(usersFilePath(), target))
        qWarning() << "UserManager: can't keep the damaged users.json as" << target;
}

bool UserManager::restoreFromBackup()
{
    for (int n = 1; n <= kBackupCount; ++n) {
        const QByteArray json = readFile(backupFilePath(n));
        if (!loadFrom(json))
            continue;
        keepDamagedFile();
        writeFileAtomically(usersFilePath(), json);
        qInfo() << "UserManager: restored users.json from" << backupFilePath(n);
        emit usersChanged();
        emit activeUserChanged();
        emit statsChanged();
        emit lichessChanged();
        emit pendingChallengeChanged();
        return true;
    }
    return false;
}

void UserManager::save()
{
    keepDamagedFile(); // the user went on without restoring

    QJsonArray usersArray;
    for (const auto& u : _users) {
        QJsonObject obj = userToJson(u);
        obj[QStringLiteral("lichessToken")]       = u.lichessTokenEncrypted;
        if (u.lichessExpired)
            obj[QStringLiteral("lichessExpired")] = true;
        obj[QStringLiteral("pendingChallengeId")]  = u.pendingChallengeId;
        obj[QStringLiteral("pendingChallengeUrl")] = u.pendingChallengeUrl;
        usersArray.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("version")]      = 2; // 2: statistics per opponent
    root[QStringLiteral("activeUserId")] = _activeUserId;
    root[QStringLiteral("users")]        = usersArray;

    writeFileAtomically(usersFilePath(), QJsonDocument(root).toJson());
}

// ── Export / import ───────────────────────────────────────────────────────

QString UserManager::exportFileName(const QString& userId) const
{
    QString name = nameOf(userId);
    name.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
    name = name.trimmed();
    return (name.isEmpty() ? QStringLiteral("profile") : name) + QStringLiteral(".chessprofile");
}

QString UserManager::exportProfile(const QString& userId, const QUrl& file)
{
    const auto* u = userById(userId);
    if (!u) return QStringLiteral("There is no such profile");
    if (!file.isLocalFile()) return QStringLiteral("Choose a file on this computer");

    QJsonObject files;
    for (const auto& name : kProfileFiles) {
        const QByteArray data = readFile(userDir(userId) + "/" + name);
        if (!data.isEmpty())
            files[name] = QString::fromLatin1(data.toBase64());
    }
    const QJsonObject root{
        { QStringLiteral("format"),   kExportFormat },
        { QStringLiteral("version"),  kExportVersion },
        { QStringLiteral("exported"), QDateTime::currentDateTime().toString(Qt::ISODate) },
        { QStringLiteral("profile"),  userToJson(*u) },
        { QStringLiteral("files"),    files },
    };
    QString error;
    if (!writeFileAtomically(file.toLocalFile(), QJsonDocument(root).toJson(), &error))
        return QStringLiteral("Couldn't save the file: ") + error;
    return {};
}

// The profile in an export file, with its saves (file name → contents). Returns "" or an error.
static QString readExport(const QUrl& file, QJsonObject& profile, QMap<QString, QByteArray>& files)
{
    if (!file.isLocalFile()) return QStringLiteral("Choose a file on this computer");
    QFile f(file.toLocalFile());
    if (!f.open(QIODevice::ReadOnly)) return QStringLiteral("Couldn't open the file: ") + f.errorString();
    const QJsonObject root = parseObject(f.readAll());
    if (root.value(QStringLiteral("format")).toString() != kExportFormat)
        return QStringLiteral("This isn't a Chess++ profile");
    if (root.value(QStringLiteral("version")).toInt() > UserManager::kExportVersion)
        return QStringLiteral("This profile comes from a newer Chess++; update the app to import it");
    profile = root.value(QStringLiteral("profile")).toObject();
    if (profile.value(QStringLiteral("id")).toString().isEmpty() || profile.value(QStringLiteral("name")).toString().trimmed().isEmpty())
        return QStringLiteral("The profile in this file is incomplete");
    const QJsonObject filesObj = root.value(QStringLiteral("files")).toObject();
    for (const auto& name : kProfileFiles) // only these names: nothing else is written
        if (filesObj.contains(name))
            files[name] = QByteArray::fromBase64(filesObj.value(name).toString().toLatin1());
    return {};
}

QVariantMap UserManager::inspectImport(const QUrl& file) const
{
    QJsonObject profile;
    QMap<QString, QByteArray> files;
    const QString error = readExport(file, profile, files);
    if (!error.isEmpty())
        return { { QStringLiteral("error"), error } };

    const QString id   = profile.value(QStringLiteral("id")).toString();
    const QString name = profile.value(QStringLiteral("name")).toString().trimmed();
    const User* conflict = userById(id);
    for (const auto& u : _users)
        if (!conflict && u.name.compare(name, Qt::CaseInsensitive) == 0)
            conflict = &u;
    return { { QStringLiteral("error"), QString() }, { QStringLiteral("name"), name },
             { QStringLiteral("conflictId"),   conflict ? conflict->id   : QString() },
             { QStringLiteral("conflictName"), conflict ? conflict->name : QString() } };
}

QString UserManager::importProfile(const QUrl& file, bool replace)
{
    const QVariantMap info = inspectImport(file);
    if (!info.value(QStringLiteral("error")).toString().isEmpty())
        return info.value(QStringLiteral("error")).toString();
    QJsonObject profile;
    QMap<QString, QByteArray> files;
    readExport(file, profile, files);

    // Through the users.json reader, so an import gets the same defaults and migrations.
    const QList<User> read = usersFromJson(QJsonArray{ profile });
    if (read.isEmpty())
        return QStringLiteral("The profile in this file is incomplete");
    User imported = read.first();
    imported.name = imported.name.trimmed();
    imported.lichessExpired = false; // no token: the profile asks to reconnect Lichess

    const QString conflictId = info.value(QStringLiteral("conflictId")).toString();
    if (replace && !conflictId.isEmpty()) {
        if (_isLoggedIn && conflictId == _activeUserId)
            return QStringLiteral("This profile is in use. Switch user first, then import it.");
        imported.id = conflictId;
        QDir(userDir(conflictId)).removeRecursively();
        *userById(conflictId) = imported;
    } else {
        imported.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString base = imported.name;
        auto taken = [this](const QString& name) {
            return std::any_of(_users.cbegin(), _users.cend(),
                [&](const User& u) { return u.name.compare(name, Qt::CaseInsensitive) == 0; });
        };
        for (int n = 2; taken(imported.name); ++n)
            imported.name = QStringLiteral("%1 (%2)").arg(base).arg(n);
        _users.append(imported);
    }
    for (auto it = files.cbegin(); it != files.cend(); ++it)
        writeFileAtomically(userDir(imported.id) + "/" + it.key(), it.value());
    save();
    emit usersChanged();
    if (imported.id == _activeUserId) {
        emit activeUserChanged();
        emit statsChanged();
        emit lichessChanged();
    }
    return {};
}
