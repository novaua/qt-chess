#include "stdafx.h"
#include "UserManager.h"
#include "AppConfig.h"

#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

UserManager::UserManager(QObject* parent)
    : QObject(parent)
{
    load();
}

QString UserManager::usersFilePath()
{
    return AppConfig::appDataDir() + "/users.json";
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

QVariantList UserManager::users() const
{
    QVariantList result;
    for (const auto& u : _users) {
        QVariantMap m;
        m[QStringLiteral("id")]        = u.id;
        m[QStringLiteral("name")]      = u.name;
        m[QStringLiteral("avatarName")] = u.avatarName;
        m[QStringLiteral("avatarUrl")]  = "qrc:/app/pics/avatars/" + u.avatarName + ".png";
        result << m;
    }
    return result;
}

int     UserManager::gamesPlayed()  const { const auto* u = activeUser(); return u ? u->gamesPlayed  : 0; }
int     UserManager::humanWins()    const { const auto* u = activeUser(); return u ? u->humanWins    : 0; }
int     UserManager::computerWins() const { const auto* u = activeUser(); return u ? u->computerWins : 0; }

bool    UserManager::lichessConnected()     const { const auto* u = activeUser(); return u && !u->lichessTokenEncrypted.isEmpty(); }
QString UserManager::lichessUsername()      const { const auto* u = activeUser(); return u ? u->lichessUsername       : QString(); }
QString UserManager::lichessTokenEncrypted() const { const auto* u = activeUser(); return u ? u->lichessTokenEncrypted : QString(); }

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
    for (const auto& u : std::as_const(_users)) {
        if (u.id == userId) {
            _activeUserId = userId;
            _isLoggedIn   = true;
            save();
            emit activeUserChanged();
            emit statsChanged();
            return;
        }
    }
}

void UserManager::logout()
{
    _isLoggedIn = false;
    // _activeUserId kept so the login screen can highlight the last user
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

UserManager::GameSaveInfo UserManager::autoSaveInfo() const
{
    const auto* u = activeUser();
    return u ? u->autoSave : GameSaveInfo{};
}

void UserManager::setAutoSaveInfo(const GameSaveInfo& info)
{
    auto* u = activeUser();
    if (!u) return;
    u->autoSave = info;
    save();
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
    u->lichessTokenEncrypted = LichessClient::encryptToken(plainToken);
    u->lichessUsername       = username;
    save();
    emit lichessChanged();
}

void UserManager::clearLichessToken()
{
    auto* u = activeUser();
    if (!u) return;
    u->lichessTokenEncrypted.clear();
    u->lichessUsername.clear();
    save();
    emit lichessChanged();
}

void UserManager::clearAutoSaveInfo()
{
    auto* u = activeUser();
    if (!u) return;
    u->autoSave = GameSaveInfo{};
    save();
}

void UserManager::recordResult(bool humanWon, bool isComputerGame)
{
    auto* u = activeUser();
    if (!u) return;
    u->gamesPlayed++;
    if (isComputerGame) {
        if (humanWon) u->humanWins++;
        else          u->computerWins++;
    }
    save();
    emit statsChanged();
}

UserManager::User* UserManager::activeUser()
{
    for (auto& u : _users)
        if (u.id == _activeUserId) return &u;
    return nullptr;
}

const UserManager::User* UserManager::activeUser() const
{
    for (const auto& u : _users)
        if (u.id == _activeUserId) return &u;
    return nullptr;
}

static QJsonObject saveInfoToJson(const UserManager::GameSaveInfo& info)
{
    QJsonObject o;
    o[QStringLiteral("isSinglePlayer")]   = info.isSinglePlayer;
    o[QStringLiteral("playerPlaysWhite")] = info.playerPlaysWhite;
    o[QStringLiteral("playerAvatarName")]  = info.playerAvatarName;
    o[QStringLiteral("opponentAvatarName")] = info.opponentAvatarName;
    return o;
}

static UserManager::GameSaveInfo saveInfoFromJson(const QJsonObject& o)
{
    UserManager::GameSaveInfo info;
    info.isSinglePlayer    = o.value(QStringLiteral("isSinglePlayer")).toBool(false);
    info.playerPlaysWhite  = o.value(QStringLiteral("playerPlaysWhite")).toBool(true);
    info.playerAvatarName  = o.value(QStringLiteral("playerAvatarName")).toString(QStringLiteral("wizard"));
    info.opponentAvatarName = o.value(QStringLiteral("opponentAvatarName")).toString(QStringLiteral("unicorn"));
    return info;
}

void UserManager::load()
{
    QFile file(usersFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

    const QJsonObject root = doc.object();
    _activeUserId = root.value(QStringLiteral("activeUserId")).toString();
    _isLoggedIn   = !_activeUserId.isEmpty();

    for (const auto& val : root.value(QStringLiteral("users")).toArray()) {
        const QJsonObject obj = val.toObject();
        User u;
        u.id         = obj.value(QStringLiteral("id")).toString();
        u.name       = obj.value(QStringLiteral("name")).toString();
        u.avatarName = obj.value(QStringLiteral("avatarName")).toString(QStringLiteral("wizard"));
        u.createdDate = QDate::fromString(obj.value(QStringLiteral("createdDate")).toString(), Qt::ISODate);
        if (!u.createdDate.isValid()) u.createdDate = QDate::currentDate();
        const QJsonObject stats = obj.value(QStringLiteral("stats")).toObject();
        u.gamesPlayed  = stats.value(QStringLiteral("gamesPlayed")).toInt(0);
        u.humanWins    = stats.value(QStringLiteral("humanWins")).toInt(0);
        u.computerWins = stats.value(QStringLiteral("computerWins")).toInt(0);
        u.autoSave              = saveInfoFromJson(obj.value(QStringLiteral("autoSave")).toObject());
        u.savedGame             = saveInfoFromJson(obj.value(QStringLiteral("savedGame")).toObject());
        u.lichessTokenEncrypted = obj.value(QStringLiteral("lichessToken")).toString();
        u.lichessUsername       = obj.value(QStringLiteral("lichessUsername")).toString();
        if (!u.id.isEmpty() && !u.name.isEmpty())
            _users.append(u);
    }
}

void UserManager::save() const
{
    QJsonArray usersArray;
    for (const auto& u : _users) {
        QJsonObject stats;
        stats[QStringLiteral("gamesPlayed")]  = u.gamesPlayed;
        stats[QStringLiteral("humanWins")]    = u.humanWins;
        stats[QStringLiteral("computerWins")] = u.computerWins;

        QJsonObject obj;
        obj[QStringLiteral("id")]          = u.id;
        obj[QStringLiteral("name")]        = u.name;
        obj[QStringLiteral("avatarName")]  = u.avatarName;
        obj[QStringLiteral("createdDate")] = u.createdDate.toString(Qt::ISODate);
        obj[QStringLiteral("stats")]       = stats;
        obj[QStringLiteral("autoSave")]        = saveInfoToJson(u.autoSave);
        obj[QStringLiteral("savedGame")]       = saveInfoToJson(u.savedGame);
        obj[QStringLiteral("lichessToken")]    = u.lichessTokenEncrypted;
        obj[QStringLiteral("lichessUsername")] = u.lichessUsername;
        usersArray.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("version")]      = 1;
    root[QStringLiteral("activeUserId")] = _activeUserId;
    root[QStringLiteral("users")]        = usersArray;

    QFile file(usersFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson());
}
