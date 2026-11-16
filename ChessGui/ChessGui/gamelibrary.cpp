#include "stdafx.h"
#include "gamelibrary.h"

#include "AppConfig.h"
#include "Game.h"
#include "UserManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>
#include <algorithm>

// ── GameRecord ───────────────────────────────────────────────────────────────

QJsonObject GameRecord::toJson() const
{
	QJsonObject o;
	o[QStringLiteral("id")]               = id;
	o[QStringLiteral("kind")]             = kind;
	o[QStringLiteral("finished")]         = finished;
	o[QStringLiteral("startedAt")]        = startedAt.toString(Qt::ISODate);
	if (finishedAt.isValid())
		o[QStringLiteral("finishedAt")]   = finishedAt.toString(Qt::ISODate);
	o[QStringLiteral("playerPlaysWhite")] = playerPlaysWhite;
	o[QStringLiteral("opponentName")]     = opponentName;
	if (!opponentUserId.isEmpty())
		o[QStringLiteral("opponentUserId")] = opponentUserId;
	if (level > 0)
		o[QStringLiteral("level")]        = level;
	o[QStringLiteral("playerAvatar")]     = playerAvatar;
	o[QStringLiteral("opponentAvatar")]   = opponentAvatar;
	if (clockLimitSec > 0 || clockIncSec > 0) {
		o[QStringLiteral("clockLimitSec")] = clockLimitSec;
		o[QStringLiteral("clockIncSec")]   = clockIncSec;
		o[QStringLiteral("whiteMs")]       = whiteMs;
		o[QStringLiteral("blackMs")]       = blackMs;
	}
	o[QStringLiteral("moves")]            = QJsonArray::fromStringList(moves);
	o[QStringLiteral("fen")]              = fen;
	if (!result.isEmpty()) {
		o[QStringLiteral("result")]       = result;
		o[QStringLiteral("resultText")]   = resultText;
	}
	if (resultRecorded)
		o[QStringLiteral("resultRecorded")] = true;
	if (undoCount > 0)
		o[QStringLiteral("undoCount")]    = undoCount;
	if (!forkOfId.isEmpty()) {
		o[QStringLiteral("forkOfId")]     = forkOfId;
		o[QStringLiteral("forkPly")]      = forkPly;
	}
	if (!analysis.isEmpty())
		o[QStringLiteral("analysis")]     = analysis;
	return o;
}

GameRecord GameRecord::fromJson(const QJsonObject& o)
{
	GameRecord r;
	r.id               = o.value(QStringLiteral("id")).toString();
	r.kind             = o.value(QStringLiteral("kind")).toString(QStringLiteral("local"));
	r.finished         = o.value(QStringLiteral("finished")).toBool(false);
	r.startedAt        = QDateTime::fromString(o.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);
	r.finishedAt       = QDateTime::fromString(o.value(QStringLiteral("finishedAt")).toString(), Qt::ISODate);
	r.playerPlaysWhite = o.value(QStringLiteral("playerPlaysWhite")).toBool(true);
	r.opponentName     = o.value(QStringLiteral("opponentName")).toString();
	r.opponentUserId   = o.value(QStringLiteral("opponentUserId")).toString();
	r.level            = o.value(QStringLiteral("level")).toInt(0);
	r.playerAvatar     = o.value(QStringLiteral("playerAvatar")).toString();
	r.opponentAvatar   = o.value(QStringLiteral("opponentAvatar")).toString();
	r.clockLimitSec    = o.value(QStringLiteral("clockLimitSec")).toInt(0);
	r.clockIncSec      = o.value(QStringLiteral("clockIncSec")).toInt(0);
	r.whiteMs          = o.value(QStringLiteral("whiteMs")).toInteger(0);
	r.blackMs          = o.value(QStringLiteral("blackMs")).toInteger(0);
	for (const auto& m : o.value(QStringLiteral("moves")).toArray())
		r.moves.append(m.toString());
	r.fen              = o.value(QStringLiteral("fen")).toString();
	r.result           = o.value(QStringLiteral("result")).toString();
	r.resultText       = o.value(QStringLiteral("resultText")).toString();
	r.resultRecorded   = o.value(QStringLiteral("resultRecorded")).toBool(false);
	r.undoCount        = o.value(QStringLiteral("undoCount")).toInt(0);
	r.forkOfId         = o.value(QStringLiteral("forkOfId")).toString();
	r.forkPly          = o.value(QStringLiteral("forkPly")).toInt(0);
	r.analysis         = o.value(QStringLiteral("analysis")).toObject();
	return r;
}

bool GameRecord::setMoves(const QStringList& uciMoves)
{
	std::vector<std::string> uci;
	for (const auto& m : uciMoves)
		uci.push_back(m.toStdString());
	auto game = std::make_shared<Chess::Game>();
	try {
		game->LoadMoves(uci);
		game->ResumeFromLoad();
	}
	catch (const std::exception&) {
		return false;
	}
	moves = uciMoves;
	fen = QString::fromStdString(game->MakeFen());
	return true;
}

QVariantMap GameRecord::toVariant() const
{
	return QVariantMap{
		{ QStringLiteral("id"),               id },
		{ QStringLiteral("kind"),             kind },
		{ QStringLiteral("finished"),         finished },
		{ QStringLiteral("startedAt"),        startedAt },
		{ QStringLiteral("finishedAt"),       finishedAt },
		{ QStringLiteral("playerPlaysWhite"), playerPlaysWhite },
		{ QStringLiteral("playerToMove"),     playerToMove() },
		{ QStringLiteral("opponentName"),     opponentName },
		{ QStringLiteral("opponentUserId"),   opponentUserId },
		{ QStringLiteral("level"),            level },
		{ QStringLiteral("clockLimitSec"),    clockLimitSec },
		{ QStringLiteral("clockIncSec"),      clockIncSec },
		{ QStringLiteral("plyCount"),         plyCount() },
		{ QStringLiteral("lastMove"),         lastMove() },
		{ QStringLiteral("fen"),              fen },
		{ QStringLiteral("result"),           result },
		{ QStringLiteral("resultText"),       resultText },
		{ QStringLiteral("undoCount"),        undoCount },
		{ QStringLiteral("forkOfId"),         forkOfId },
		{ QStringLiteral("forkPly"),          forkPly },
		{ QStringLiteral("analysed"),         !analysis.isEmpty() },
	};
}

// ── GameLibrary ──────────────────────────────────────────────────────────────

GameLibrary::GameLibrary(QObject* parent)
	: QObject(parent)
{
}

void GameLibrary::setUserManager(UserManager* um)
{
	_userManager = um;
	connect(_userManager, &UserManager::activeUserChanged, this, &GameLibrary::reload);
	reload();
}

QString GameLibrary::filePathOf(const QString& userId)
{
	return AppConfig::gamesFilePath(userId);
}

QString GameLibrary::filePath() const
{
	return filePathOf(_userId);
}

static bool newerFirst(const GameRecord& a, const GameRecord& b)
{
	return a.startedAt > b.startedAt;
}

// The active profile's games. Without one (logged out, deleted) the library is empty and keeps nothing.
void GameLibrary::reload()
{
	const QString userId = _userManager && _userManager->isLoggedIn() ? _userManager->activeUserId() : QString();
	_userId = userId;
	_records.clear();
	if (!_userId.isEmpty()) {
		QFile file(filePath());
		if (file.open(QIODevice::ReadOnly)) {
			QJsonParseError err;
			const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject()) {
				// Kept aside, not overwritten: the owner may want to look at it.
				const QString aside = filePath() + QStringLiteral(".damaged");
				QFile::remove(aside);
				file.close();
				QFile::rename(filePath(), aside);
				qWarning() << "GameLibrary: games.json is damaged, kept as" << aside << err.errorString();
			} else {
				for (const auto& g : doc.object().value(QStringLiteral("games")).toArray()) {
					GameRecord r = GameRecord::fromJson(g.toObject());
					if (!r.id.isEmpty() && !r.moves.isEmpty())
						_records.append(r);
				}
				std::stable_sort(_records.begin(), _records.end(), newerFirst);
			}
		} else {
			migrateOldSaves();
		}
	}
	emit gamesChanged();
}

bool GameLibrary::save()
{
	if (_userId.isEmpty())
		return false;
	QJsonArray games;
	for (const auto& r : _records)
		games.append(r.toJson());
	const QJsonObject root{
		{ QStringLiteral("version"), FormatVersion },
		{ QStringLiteral("games"),   games },
	};
	QSaveFile file(filePath());
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "GameLibrary: can't write" << filePath() << file.errorString();
		return false;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	if (!file.commit()) {
		qWarning() << "GameLibrary: can't write" << filePath() << file.errorString();
		return false;
	}
	return true;
}

QVariantList GameLibrary::games() const
{
	QVariantList list;
	for (const auto& r : _records)
		list.append(r.toVariant());
	return list;
}

QVariantList GameLibrary::unfinishedGames() const
{
	QVariantList list;
	for (const auto& r : _records)
		if (!r.finished)
			list.append(r.toVariant());
	return list;
}

int GameLibrary::unfinishedCount() const
{
	return (int)std::count_if(_records.begin(), _records.end(), [](const GameRecord& r) { return !r.finished; });
}

QList<GameRecord> GameLibrary::unfinished() const
{
	QList<GameRecord> list;
	for (const auto& r : _records)
		if (!r.finished)
			list.append(r);
	return list;
}

std::optional<GameRecord> GameLibrary::find(const QString& id) const
{
	for (const auto& r : _records)
		if (r.id == id)
			return r;
	return std::nullopt;
}

QVariantMap GameLibrary::game(const QString& id) const
{
	const auto r = find(id);
	return r ? r->toVariant() : QVariantMap{};
}

void GameLibrary::setKeepCount(int count)
{
	count = std::max(1, count);
	if (_keepCount == count)
		return;
	_keepCount = count;
	emit keepCountChanged();
	const int before = _records.size();
	trim();
	if (_records.size() != before) {
		save();
		emit gamesChanged();
	}
}

QString GameLibrary::put(GameRecord record)
{
	if (_userId.isEmpty())
		return {};
	if (record.id.isEmpty())
		record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	if (!record.startedAt.isValid())
		record.startedAt = QDateTime::currentDateTime();
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == record.id; });
	if (it != _records.end())
		*it = record;
	else
		_records.prepend(record);
	std::stable_sort(_records.begin(), _records.end(), newerFirst);
	trim();
	save();
	emit gamesChanged();
	return record.id;
}

bool GameLibrary::remove(const QString& id)
{
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
	if (it == _records.end())
		return false;
	_records.erase(it);
	save();
	emit gamesChanged();
	return true;
}

// The oldest finished games beyond keepCount go (by when they finished); unfinished ones stay.
void GameLibrary::trim()
{
	QList<const GameRecord*> finished;
	for (const auto& r : _records)
		if (r.finished)
			finished.append(&r);
	if (finished.size() <= _keepCount)
		return;
	std::sort(finished.begin(), finished.end(), [](const GameRecord* a, const GameRecord* b) {
		const QDateTime da = a->finishedAt.isValid() ? a->finishedAt : a->startedAt;
		const QDateTime db = b->finishedAt.isValid() ? b->finishedAt : b->startedAt;
		return da < db;
	});
	QStringList drop;
	for (int i = 0; i < finished.size() - _keepCount; ++i)
		drop.append(finished[i]->id);
	_records.erase(std::remove_if(_records.begin(), _records.end(),
		[&](const GameRecord& r) { return drop.contains(r.id); }), _records.end());
}

// Versions before BL-19 kept two binary files per profile, the robot's autosave and the game saved
// with Save, with their metadata in users.json (UserManager::GameSaveInfo). On the first load without
// a games.json they become records: the autosave an unfinished robot game; the saved game finished when
// it ended in mate or stalemate, else unfinished (it can go on now). The old files are then removed.
void GameLibrary::migrateOldSaves()
{
	if (!_userManager)
		return;
	struct OldSave { QString path; UserManager::GameSaveInfo info; bool autosave; };
	const OldSave saves[] = {
		{ AppConfig::autoSaveFilePath(_userId),  _userManager->autoSaveInfo(),  true },
		{ AppConfig::savedGameFilePath(_userId), _userManager->savedGameInfo(), false },
	};
	bool any = false;
	for (const auto& s : saves) {
		if (!QFileInfo::exists(s.path))
			continue;
		any = true;
		try {
			Chess::Game game;
			game.Load(s.path.toStdString());
			game.ResumeFromLoad();
			GameRecord r;
			r.kind = s.info.isSinglePlayer ? QStringLiteral("computer") : QStringLiteral("local");
			r.startedAt = s.info.startDate.isValid() ? s.info.startDate.startOfDay()
			            : QFileInfo(s.path).lastModified();
			r.playerPlaysWhite = s.info.playerPlaysWhite;
			r.opponentName = !s.info.opponentName.isEmpty() ? s.info.opponentName
			               : s.info.isSinglePlayer ? QStringLiteral("Robot") : QStringLiteral("Guest");
			r.opponentUserId = s.info.opponentUserId;
			r.level = s.info.isSinglePlayer ? s.info.computerLevel : 0;
			r.playerAvatar = s.info.playerAvatarName;
			r.opponentAvatar = s.info.opponentAvatarName;
			r.clockLimitSec = s.info.clockLimitSec;
			r.clockIncSec = s.info.clockIncSec;
			r.whiteMs = s.info.whiteMs;
			r.blackMs = s.info.blackMs;
			r.resultRecorded = s.info.resultRecorded;
			for (const auto& m : game.GetGameRecord())
				r.moves.append(QString::fromStdString(m.ToUciString()));
			r.fen = QString::fromStdString(game.MakeFen());
			if (!s.autosave) {
				const bool whiteToMove = game.IsWhiteMove();
				if (game.IsCheckMate()) {
					r.finished = true;
					r.result = whiteToMove ? QStringLiteral("0-1") : QStringLiteral("1-0");
					r.resultText = QStringLiteral("%1 wins by checkmate").arg(whiteToMove ? QStringLiteral("Black") : QStringLiteral("White"));
				} else if (game.IsStalemate()) {
					r.finished = true;
					r.result = QStringLiteral("1/2-1/2");
					r.resultText = QStringLiteral("Draw by stalemate");
				}
				if (r.finished)
					r.finishedAt = QFileInfo(s.path).lastModified();
			}
			if (!r.moves.isEmpty()) {
				r.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
				_records.append(r);
			}
		}
		catch (const std::exception& e) {
			qWarning() << "GameLibrary: can't migrate" << s.path << e.what();
		}
		QFile::remove(s.path);
	}
	if (!any)
		return;
	std::stable_sort(_records.begin(), _records.end(), newerFirst);
	_userManager->clearAutoSaveInfo();
	_userManager->clearSavedGameInfo();
	save();
	qInfo() << "GameLibrary: migrated" << _records.size() << "saved game(s) of the profile into" << filePath();
}
