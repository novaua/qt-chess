#include "stdafx.h"
#include "UserManager.h"
#include "AppConfig.h"
#include "AvatarProvider.h"
#include "LichessAuth.h"
#include "gamelibrary.h"
#include "gamesignature.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUuid>

static const QString kExportFormat = QStringLiteral("chessplusplus-profile");
// The per-user files an export carries (AppConfig::autoSaveFilePath / savedGameFilePath).
// The profile's files in an export. BL-20: its games are one signed file each (games/<id>.json, gameFiles())
// and its rating rating.json; games.json is the games library of BL-19 (version 1 exports, migrated by
// GameLibrary when the profile loads), and the two binary saves those of older exports.
static const QStringList kProfileFiles = { QStringLiteral("rating.json"), QStringLiteral("games.json"),
                                           QStringLiteral("chess.autosave"), QStringLiteral("chess.save") };

// An export's name of a game file: games/<id>.json, the id as GameLibrary names its files. Nothing
// else under games/ is written on import.
static bool isGameFileName(const QString& name)
{
    static const QString prefix = QStringLiteral("games/"), suffix = QStringLiteral(".json");
    return name.startsWith(prefix) && name.endsWith(suffix)
        && GameLibrary::isFileId(name.mid(prefix.size(), name.size() - prefix.size() - suffix.size()));
}

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

// A profile's statistics in users.json (or a backup, an export) fit their signature.
static bool statsVerify(const QJsonObject& user)
{
    return GameSignature::verify(GameSignature::Kind::Statistics, user.value(QStringLiteral("stats")).toObject(),
                                 user.value(QStringLiteral("statsSig")).toString());
}

// What load() accepts as users.json: an object with a "users" array.
static bool isUsersFile(const QByteArray& json)
{
    return parseObject(json).value(QStringLiteral("users")).isArray();
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

QString UserManager::avatarOf(const QString& userId) const
{
    const auto* u = userById(userId);
    return u ? u->avatarName : QString();
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

QVariantMap UserManager::lastImport() const
{
    return _lastImport;
}

QVariantList UserManager::takeRestoredStatistics()
{
    return std::exchange(_restoredStatistics, {});
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
    cancelLichessLink(); // a late browser sign-in must not switch or link another profile
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

QString UserManager::lichessToken()
{
    if (!lichessConnected())
        return {};
    const QString token = LichessClient::decryptToken(lichessTokenEncrypted());
    if (token.isEmpty()) {
        qWarning() << "UserManager: the Lichess sign-in of" << activeUserId()
                   << "can't be decrypted here (linked on another PC or Windows account): reconnect";
        // Expired now (lichessConnected() says so at once), but written and told once the caller is done:
        // the one asking (AppServices::updateLichessToken, on lichessChanged) isn't run again inside itself.
        activeUser()->lichessExpired = true;
        QMetaObject::invokeMethod(this, [this]() {
            save();
            emit lichessChanged();
        }, Qt::QueuedConnection);
    }
    return token;
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
                createUser(uniqueName(username), avatars[QRandomGenerator::global()->bounded((int)avatars.size())]);
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

void UserManager::clearSavedGameInfo()
{
    auto* u = activeUser();
    if (!u) return;
    u->savedGame = GameSaveInfo{};
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
    if (info.computerLevel > 0)
        o[QStringLiteral("computerLevel")]  = info.computerLevel;
    if (info.resultRecorded)
        o[QStringLiteral("resultRecorded")] = true;
    if (info.clockLimitSec > 0 || info.clockIncSec > 0) {
        o[QStringLiteral("clockLimitSec")]  = info.clockLimitSec;
        o[QStringLiteral("clockIncSec")]    = info.clockIncSec;
        o[QStringLiteral("whiteMs")]        = info.whiteMs;
        o[QStringLiteral("blackMs")]        = info.blackMs;
    }
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
    info.computerLevel      = o.value(QStringLiteral("computerLevel")).toInt(0);
    info.resultRecorded     = o.value(QStringLiteral("resultRecorded")).toBool(false);
    info.clockLimitSec      = o.value(QStringLiteral("clockLimitSec")).toInt(0);
    info.clockIncSec        = o.value(QStringLiteral("clockIncSec")).toInt(0);
    info.whiteMs            = o.value(QStringLiteral("whiteMs")).toInteger(0);
    info.blackMs            = o.value(QStringLiteral("blackMs")).toInteger(0);
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

void UserManager::recoverStatistics(User& u, KeptGames& kept)
{
    u.statsVerified = true; // as restored: signed at the next save
    for (int n = 1; n <= kBackupCount; ++n) {
        const QJsonObject root = parseObject(readFile(backupFilePath(n)));
        if (root.value(QStringLiteral("version")).toInt() < 3)
            continue; // its statistics were never signed
        for (const auto& val : root.value(QStringLiteral("users")).toArray()) {
            const QJsonObject obj = val.toObject();
            if (obj.value(QStringLiteral("id")).toString() != u.id || !statsVerify(obj))
                continue;
            u.stats = statsFromJson(obj.value(QStringLiteral("stats")).toObject());
            qWarning() << "UserManager: the statistics of" << u.id << "failed their check; restored from" << backupFilePath(n);
            _restoredStatistics.append(QVariantMap{ { QStringLiteral("name"), u.name }, { QStringLiteral("rebuilt"), false } });
            return;
        }
    }
    u.stats = statisticsFromGames(u.id, kept);
    qWarning() << "UserManager: the statistics of" << u.id << "failed their check and no backup verifies; rebuilt from the kept games";
    _restoredStatistics.append(QVariantMap{ { QStringLiteral("name"), u.name }, { QStringLiteral("rebuilt"), true } });
}

// The rows ChessConnector::recordResult made for the kept games that verify: the profile's own (vs the
// robot by level, Guest, another profile), and the two-player games another profile kept against this
// one (the profile signed in keeps the game). Lichess games have no record.
QMap<QString, UserManager::Record> UserManager::statisticsFromGames(const QString& userId, KeptGames& kept) const
{
    // Each profile's games read once, however many profiles are rebuilt.
    auto keptOf = [&kept](const QString& id) -> const QList<GameRecord>& {
        auto it = kept.find(id);
        if (it == kept.end())
            it = kept.insert(id, GameLibrary::keptRecordsOf(id));
        return *it;
    };
    QMap<QString, Record> stats;
    auto add = [&stats](const QString& key, const QString& name, double score) {
        Record& r = stats[key];
        r.opponentName = name;
        r.games++;
        if (score > 0.5)      r.wins++;
        else if (score < 0.5) r.losses++;
        else                  r.draws++;
    };
    auto counted = [](const GameRecord& r) { return r.finished && r.resultRecorded && !r.result.isEmpty(); };
    for (const GameRecord& r : keptOf(userId)) {
        if (!counted(r))
            continue;
        if (r.kind == QLatin1String("computer"))
            add(computerKey(r.level), QStringLiteral("Robot"), r.playerScore());
        else if (r.opponentUserId.isEmpty())
            add(QStringLiteral("guest"), QStringLiteral("Guest"), r.playerScore());
        else
            add(userKey(r.opponentUserId), r.opponentName, r.playerScore());
    }
    for (const User& other : _users) {
        if (other.id == userId)
            continue;
        for (const GameRecord& r : keptOf(other.id))
            if (counted(r) && r.kind == QLatin1String("local") && r.opponentUserId == userId)
                add(userKey(other.id), other.name, 1.0 - r.playerScore());
    }
    return stats;
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
    const QJsonObject stats = statsToJson(u.stats);
    obj[QStringLiteral("stats")]       = stats;
    obj[QStringLiteral("statsSig")]    = GameSignature::sign(GameSignature::Kind::Statistics, stats);
    obj[QStringLiteral("autoSave")]    = saveInfoToJson(u.autoSave);
    obj[QStringLiteral("savedGame")]   = saveInfoToJson(u.savedGame);
    obj[QStringLiteral("lichessUsername")] = u.lichessUsername;
    if (!u.lichessRatings.isEmpty())
        obj[QStringLiteral("lichessRatings")] = QJsonObject::fromVariantMap(u.lichessRatings);
    return obj;
}

QList<UserManager::User> UserManager::usersFromJson(const QJsonArray& array, bool signedStats)
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
        u.statsVerified = !signedStats || statsVerify(obj);
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

bool UserManager::loadFrom(const QByteArray& json, bool* restored)
{
    if (!isUsersFile(json))
        return false;
    const QJsonObject root = parseObject(json);
    const QJsonArray users = root.value(QStringLiteral("users")).toArray();
    // Before version 3 nothing was signed: trusted once (BL-20 §3.5), signed when it is next written
    // (load() writes it at once). Not when a profile already has signed files: this app wrote version 3
    // there, so an older one is a way around the check (review of #187-#190).
    bool signedStats = root.value(QStringLiteral("version")).toInt() >= 3;
    for (const auto& val : users)
        signedStats = signedStats || GameLibrary::hasSignedFiles(val.toObject().value(QStringLiteral("id")).toString());
    _users = usersFromJson(users, signedStats);
    bool any = false;
    KeptGames kept;
    for (User& u : _users) {
        if (!u.statsVerified) {
            recoverStatistics(u, kept);
            any = true;
        }
    }
    if (restored)
        *restored = any;
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
    bool restored = false;
    if (!loadFrom(json, &restored)) {
        qWarning() << "UserManager: users.json can't be read; it is kept until a restore or the next save";
        _damaged = true;
        return;
    }
    if (restored) {
        save(); // the restored statistics, signed; that file is the backup, not the one that failed
        rotateBackups(readFile(usersFilePath()));
        return;
    }
    rotateBackups(json);
    // A file from before the statistics were signed, trusted this once: signed now, so it is never
    // trusted again (once this profile's games are signed files, an older file isn't: loadFrom).
    if (parseObject(json).value(QStringLiteral("version")).toInt() < kUsersVersion)
        save();
}

// backups/users-1.json is the newest. An unchanged file isn't copied again, so the older copies stay.
void UserManager::rotateBackups(const QByteArray& json)
{
    if (readFile(backupFilePath(1)) == json)
        return;
    QFile::remove(backupFilePath(kBackupCount));
    for (int n = kBackupCount - 1; n >= 1; --n)
        QFile::rename(backupFilePath(n), backupFilePath(n + 1));
    AppConfig::writeFileAtomically(backupFilePath(1), json);
    emit backupsChanged();
}

QVariantList UserManager::backups() const
{
    QVariantList list;
    // The profiles, not who was logged in: a restore keeps the active profile.
    const QJsonValue current = parseObject(readFile(usersFilePath())).value(QStringLiteral("users"));
    for (int n = 1; n <= kBackupCount; ++n) {
        const QJsonValue users = parseObject(readFile(backupFilePath(n))).value(QStringLiteral("users"));
        if (!users.isArray())
            continue;
        QStringList names;
        for (const auto& u : usersFromJson(users.toArray()))
            names.append(u.name);
        const QDateTime saved = QFileInfo(backupFilePath(n)).lastModified();
        list.append(QVariantMap{
            { QStringLiteral("number"),   n },
            { QStringLiteral("date"),     dayText(saved.date()) + QStringLiteral(", ")
                                          + QLocale().toString(saved.time(), QLocale::ShortFormat) },
            { QStringLiteral("profiles"), names },
            { QStringLiteral("current"),  users == current },
        });
    }
    return list;
}

bool UserManager::backupNow()
{
    const QByteArray json = readFile(usersFilePath());
    if (_damaged || !isUsersFile(json))
        return false;
    rotateBackups(json);
    return readFile(backupFilePath(1)) == json;
}

bool UserManager::restoreBackup(int n)
{
    if (n < 1 || n > kBackupCount)
        return false;
    const QByteArray json = readFile(backupFilePath(n));
    if (!isUsersFile(json))
        return false;
    if (_damaged) {
        if (!keepDamagedFile())
            return false;
    } else {
        const QByteArray current = readFile(usersFilePath());
        if (isUsersFile(current))
            rotateBackups(current);
    }

    const QString userId = _activeUserId;
    const bool wasLoggedIn = _isLoggedIn;
    const QList<User> before = _users;
    bool restored = false;
    loadFrom(json, &restored);
    // What belongs to this PC now stays: the save files on disk weren't restored, and a Lichess
    // sign-in removed since the backup must not come back.
    for (User& u : _users) {
        for (const User& now : before) {
            if (now.id != u.id)
                continue;
            u.autoSave              = now.autoSave;
            u.savedGame             = now.savedGame;
            u.lichessTokenEncrypted = now.lichessTokenEncrypted;
            u.lichessUsername       = now.lichessUsername;
            u.lichessExpired        = now.lichessExpired;
            u.lichessRatings        = now.lichessRatings;
            u.pendingChallengeId    = now.pendingChallengeId;
            u.pendingChallengeUrl   = now.pendingChallengeUrl;
        }
    }
    if (wasLoggedIn && userById(userId)) {
        _activeUserId = userId;
        _isLoggedIn = true;
    }
    save();
    qInfo() << "UserManager: restored users.json from backup" << n;
    emitAllChanged();
    if (restored)
        emit statisticsRestored();
    return true;
}

bool UserManager::canRestoreBackup() const
{
    if (!_damaged) return false;
    for (int n = 1; n <= kBackupCount; ++n)
        if (isUsersFile(readFile(backupFilePath(n))))
            return true;
    return false;
}

// users.damaged-<time>.json: never overwrite a file the user may still want to recover. A copy, so
// a locked users.json doesn't matter; if even the copy fails, the caller must not write users.json.
bool UserManager::keepDamagedFile()
{
    if (!_damaged) return true;
    const QString base = AppConfig::appDataDir() + "/users.damaged-"
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    QString target = base + ".json";
    for (int n = 2; QFile::exists(target); ++n)
        target = base + QStringLiteral("-%1.json").arg(n);
    if (!QFile::copy(usersFilePath(), target)) {
        qWarning() << "UserManager: can't keep the damaged users.json as" << target << "- not overwriting it";
        return false;
    }
    _damaged = false;
    return true;
}

void UserManager::emitAllChanged()
{
    emit usersChanged();
    emit activeUserChanged();
    emit statsChanged();
    emit lichessChanged();
    emit pendingChallengeChanged();
}

bool UserManager::restoreFromBackup()
{
    for (int n = 1; n <= kBackupCount; ++n) {
        const QByteArray json = readFile(backupFilePath(n));
        if (!isUsersFile(json))
            continue;
        if (!keepDamagedFile())
            return false;
        bool restored = false;
        loadFrom(json, &restored);
        if (restored)
            save(); // its statistics restored, signed
        else
            AppConfig::writeFileAtomically(usersFilePath(), json);
        qInfo() << "UserManager: restored users.json from" << backupFilePath(n);
        emitAllChanged();
        if (restored)
            emit statisticsRestored();
        return true;
    }
    return false;
}

void UserManager::save()
{
    if (!keepDamagedFile()) // the user went on without restoring
        return;

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
    root[QStringLiteral("version")]      = kUsersVersion;
    root[QStringLiteral("activeUserId")] = _activeUserId;
    root[QStringLiteral("users")]        = usersArray;

    AppConfig::writeFileAtomically(usersFilePath(), QJsonDocument(root).toJson());
    emit backupsChanged(); // which backup is "the same as now" may have changed
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
    QStringList names = kProfileFiles;
    for (const auto& game : QDir(userDir(userId) + "/games").entryList({ QStringLiteral("*.json") }, QDir::Files))
        names.append(QStringLiteral("games/") + game); // signed as they are: they verify on import (BL-20)
    for (const auto& name : names) {
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
    if (!AppConfig::writeFileAtomically(file.toLocalFile(), QJsonDocument(root).toJson(), &error))
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
    for (auto it = filesObj.begin(); it != filesObj.end(); ++it) // only these names: nothing else is written
        if (kProfileFiles.contains(it.key()) || isGameFileName(it.key()))
            files[it.key()] = QByteArray::fromBase64(it.value().toString().toLatin1());
    return {};
}

QString UserManager::readImport(const QUrl& file, User& user, QMap<QString, QByteArray>& files,
    const User*& conflict) const
{
    QJsonObject profile;
    const QString error = readExport(file, profile, files);
    if (!error.isEmpty())
        return error;
    // Through the users.json reader, so an import gets the same defaults and migrations; its statistics
    // checked (statsVerified: an export from before BL-20 has no signature).
    const QList<User> read = usersFromJson(QJsonArray{ profile }, true);
    if (read.isEmpty())
        return QStringLiteral("The profile in this file is incomplete");
    user = read.first();
    user.name = user.name.trimmed();
    conflict = userById(user.id);
    for (const auto& u : _users)
        if (!conflict && u.name.compare(user.name, Qt::CaseInsensitive) == 0)
            conflict = &u;
    return {};
}

QVariantMap UserManager::inspectImport(const QUrl& file) const
{
    User user;
    QMap<QString, QByteArray> files;
    const User* conflict = nullptr;
    const QString error = readImport(file, user, files, conflict);
    if (!error.isEmpty())
        return { { QStringLiteral("error"), error } };
    return { { QStringLiteral("error"), QString() }, { QStringLiteral("name"), user.name },
             { QStringLiteral("conflictId"),   conflict ? conflict->id   : QString() },
             { QStringLiteral("conflictName"), conflict ? conflict->name : QString() } };
}

QString UserManager::uniqueName(const QString& name) const
{
    auto taken = [this](const QString& candidate) {
        return std::any_of(_users.cbegin(), _users.cend(),
            [&](const User& u) { return u.name.compare(candidate, Qt::CaseInsensitive) == 0; });
    };
    QString result = name;
    for (int n = 2; taken(result); ++n)
        result = QStringLiteral("%1 (%2)").arg(name).arg(n);
    return result;
}

QString UserManager::importProfile(const QUrl& file, bool replace)
{
    User imported;
    QMap<QString, QByteArray> files;
    const User* conflict = nullptr;
    const QString error = readImport(file, imported, files, conflict);
    if (!error.isEmpty())
        return error;
    imported.lichessExpired = false; // no token: the profile asks to reconnect Lichess

    const QString conflictId = conflict ? conflict->id : QString();
    if (replace && !conflictId.isEmpty()) {
        if (_isLoggedIn && conflictId == _activeUserId)
            return QStringLiteral("This profile is in use. Switch user first, then import it.");
        imported.id = conflictId;
        QDir(userDir(conflictId)).removeRecursively();
        *userById(conflictId) = imported;
    } else {
        imported.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        imported.name = uniqueName(imported.name);
        _users.append(imported);
    }
    // BL-20 step 3: the games and the rating checked before they are written (GameLibrary::checkImport).
    const GameLibrary::ImportCheck check = GameLibrary::checkImport(files);
    for (auto it = files.cbegin(); it != files.cend(); ++it)
        AppConfig::writeFileAtomically(userDir(imported.id) + "/" + it.key(), it.value());
    // Statistics that don't verify (edited, or an export from before BL-20): rebuilt from its games.
    const bool statsRebuilt = !imported.statsVerified;
    if (statsRebuilt) {
        User* u = userById(imported.id);
        KeptGames kept;
        u->stats = statisticsFromGames(imported.id, kept);
        u->statsVerified = true;
        qWarning() << "UserManager: the imported statistics of" << imported.name << "failed their check; rebuilt from its games";
    }
    _lastImport = QVariantMap{
        { QStringLiteral("setAside"),       check.setAside },
        { QStringLiteral("unrated"),        check.unrated },
        { QStringLiteral("ratingRestored"), check.ratingRestored },
        { QStringLiteral("statsRebuilt"),   statsRebuilt },
    };
    save();
    emit usersChanged();
    if (imported.id == _activeUserId) {
        emit activeUserChanged();
        emit statsChanged();
        emit lichessChanged();
    }
    return {};
}
