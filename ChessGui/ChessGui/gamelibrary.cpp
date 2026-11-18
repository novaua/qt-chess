#include "stdafx.h"
#include "gamelibrary.h"

#include "AppConfig.h"
#include "EngineLevel.h"
#include "Game.h"
#include "gameanalysis.h"
#include "UserManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTimer>
#include <QUuid>
#include <algorithm>
#include <cmath>

// ── LocalRating ──────────────────────────────────────────────────────────────

int LocalRating::changeFor(int opponentElo, double score) const
{
	const double expected = 1.0 / (1.0 + std::pow(10.0, (opponentElo - _value) / 400.0));
	return (int)std::lround(kFactor() * (score - expected));
}

QJsonObject LocalRating::toJson() const
{
	return QJsonObject{ { QStringLiteral("value"), _value }, { QStringLiteral("games"), _games } };
}

LocalRating LocalRating::fromJson(const QJsonObject& o)
{
	if (o.isEmpty())
		return {};
	return LocalRating(o.value(QStringLiteral("value")).toInt(Start), o.value(QStringLiteral("games")).toInt(0));
}

// ── GameRecord ───────────────────────────────────────────────────────────────

bool GameRecord::rateable() const
{
	return kind == QLatin1String("computer") && finished && forkOfId.isEmpty() && undoCount == 0 && !chess960()
		&& plyCount() >= 2 && level >= 1 && level <= Chess::EngineLevel::Count
		&& (result == QLatin1String("1-0") || result == QLatin1String("0-1") || result == QLatin1String("1/2-1/2"));
}

double GameRecord::playerScore() const
{
	if (result == QLatin1String("1/2-1/2"))
		return 0.5;
	return (result == QLatin1String("1-0")) == playerPlaysWhite ? 1.0 : 0.0;
}

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
	if (chess960()) {
		o[QStringLiteral("variant")]      = QStringLiteral("chess960");
		o[QStringLiteral("startFen")]     = startFen;
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
	if (rated) {
		o[QStringLiteral("ratingBefore")] = ratingBefore;
		o[QStringLiteral("ratingChange")] = ratingChange;
	}
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
	if (o.value(QStringLiteral("variant")).toString() == QLatin1String("chess960")) {
		// Kept as read, also when it can't be read: never downgraded to a standard game (its moves are
		// Chess960 moves) nor lost at the next write.
		r.startFen = o.value(QStringLiteral("startFen")).toString();
		r.start = Chess::StartPosition::FromFen(r.startFen.toStdString());
		if (!r.start)
			qWarning() << "GameLibrary: game" << r.id << "has a Chess960 start that can't be read:" << r.startFen;
	}
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
	r.rated            = o.contains(QStringLiteral("ratingChange"));
	r.ratingBefore     = o.value(QStringLiteral("ratingBefore")).toInt(0);
	r.ratingChange     = o.value(QStringLiteral("ratingChange")).toInt(0);
	return r;
}

bool GameRecord::playOn(Chess::Game& game, const QStringList& uciMoves)
{
	for (const auto& m : uciMoves) {
		try {
			const auto move = Chess::Move::Parse(m.toStdString());
			if (game.GetPieceAt(move.From).IsEmpty())
				return false;
			game.DoMove(move);
		}
		catch (const std::exception&) {
			return false;
		}
	}
	return true;
}

std::shared_ptr<Chess::Game> GameRecord::play(const QStringList& uciMoves, const Chess::StartPosition& start)
{
	auto game = std::make_shared<Chess::Game>();
	game->Restart(true, start);
	return playOn(*game, uciMoves) ? game : nullptr;
}

std::shared_ptr<Chess::Game> GameRecord::replay(int plies) const
{
	if (!start)
		return nullptr; // a Chess960 start that can't be read
	return play(plies < 0 ? moves : moves.mid(0, plies), *start);
}

void GameRecord::setStartPosition(const Chess::StartPosition& position)
{
	start = position;
	startFen = position.IsChess960() ? QString::fromStdString(position.Fen()) : QString();
}

bool GameRecord::setMoves(const QStringList& uciMoves)
{
	if (!start)
		return false;
	const auto game = play(uciMoves, *start);
	if (!game)
		return false;
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
		{ QStringLiteral("chess960"),         chess960() },
		{ QStringLiteral("startPosition"),    chess960() && start ? start->Number() : -1 }, // -1: standard, or unreadable
		{ QStringLiteral("lastMove"),         lastMove() },
		{ QStringLiteral("fen"),              fen },
		{ QStringLiteral("result"),           result },
		{ QStringLiteral("resultText"),       resultText },
		{ QStringLiteral("undoCount"),        undoCount },
		{ QStringLiteral("forkOfId"),         forkOfId },
		{ QStringLiteral("forkPly"),          forkPly },
		{ QStringLiteral("analysed"),         !analysis.isEmpty() },
		{ QStringLiteral("rated"),            rated },
		{ QStringLiteral("ratingBefore"),     ratingBefore },
		{ QStringLiteral("ratingChange"),     ratingChange },
	};
}

// ── GameLibrary ──────────────────────────────────────────────────────────────

GameLibrary::GameLibrary(QObject* parent)
	: QObject(parent)
{
}

GameLibrary::~GameLibrary()
{
	flush(); // e.g. the game kept as the app closes (ChessConnector goes first)
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
GameLibrary::FileState GameLibrary::readFile(const QString& path, ProfileFile& profileFile)
{
	auto& records = profileFile.records;
	records.clear();
	profileFile.rating = {};
	if (!QFileInfo::exists(path))
		return FileState::Missing;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return FileState::Unreadable;
	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return FileState::Damaged;
	profileFile.rating = LocalRating::fromJson(doc.object().value(QStringLiteral("rating")).toObject());
	for (const auto& g : doc.object().value(QStringLiteral("games")).toArray()) {
		GameRecord r = GameRecord::fromJson(g.toObject());
		if (!r.id.isEmpty() && !r.moves.isEmpty())
			records.append(r);
	}
	std::stable_sort(records.begin(), records.end(), newerFirst);
	return FileState::Read;
}

bool GameLibrary::writeFile(const QString& path, const ProfileFile& profileFile)
{
	QJsonArray games;
	for (const auto& r : profileFile.records)
		games.append(r.toJson());
	QJsonObject root{
		{ QStringLiteral("version"), FormatVersion },
		{ QStringLiteral("games"),   games },
	};
	if (profileFile.rating.games() > 0)
		root[QStringLiteral("rating")] = profileFile.rating.toJson();
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "GameLibrary: can't write" << path << file.errorString();
		return false;
	}
	file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
	if (!file.commit()) {
		qWarning() << "GameLibrary: can't write" << path << file.errorString();
		return false;
	}
	return true;
}

// A damaged file is kept aside, not overwritten or deleted: the owner may want to look at it.
static void keepAside(const QString& path)
{
	const QString aside = path + QStringLiteral(".damaged");
	QFile::remove(aside);
	QFile::rename(path, aside);
	qWarning() << "GameLibrary:" << path << "is damaged, kept as" << aside;
}

void GameLibrary::reload()
{
	flush(); // the profile being left: its last change
	const QString userId = _userManager && _userManager->isLoggedIn() ? _userManager->activeUserId() : QString();
	_userId = userId;
	_unreadable = false;
	_records.clear();
	_rating = {};
	if (!_userId.isEmpty()) {
		ProfileFile file;
		switch (readFile(filePath(), file)) {
		case FileState::Read:
			_records = file.records;
			_rating = file.rating;
			break;
		case FileState::Damaged:
			keepAside(filePath());
			break;
		case FileState::Unreadable:
			// There, but not to be read now (another program holds it): its games aren't shown, and it
			// isn't written over; save() tries it again.
			_unreadable = true;
			qWarning() << "GameLibrary: can't read" << filePath() << "now; it is kept as it is";
			break;
		case FileState::Missing:
			migrateOldSaves();
			break;
		}
	}
	emit gamesChanged();
	emit ratingChanged();
}

bool GameLibrary::save()
{
	_dirty = false;
	if (_userId.isEmpty())
		return false;
	if (_unreadable) {
		// Read it again: its games and the ones kept since go in together, and its rating moved by the
		// games rated since (the rating here started from the start rating: what it moved is the change).
		ProfileFile onDisk;
		switch (readFile(filePath(), onDisk)) {
		case FileState::Unreadable:
			_dirty = true; // the next change tries again
			qWarning() << "GameLibrary: still can't read" << filePath() << "- not written";
			return false;
		case FileState::Damaged:
			keepAside(filePath());
			break;
		case FileState::Read:
			for (const auto& r : onDisk.records)
				if (std::none_of(_records.begin(), _records.end(), [&](const GameRecord& k) { return k.id == r.id; }))
					_records.append(r);
			std::stable_sort(_records.begin(), _records.end(), newerFirst);
			trim();
			_rating = onDisk.rating;
			emit ratingChanged();
			break;
		case FileState::Missing:
			break;
		}
		_unreadable = false;
		// The games that finished meanwhile, rated now that the rating they move is known.
		for (const QString& id : std::exchange(_rateLater, {})) {
			const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
			if (it != _records.end())
				rate(*it);
		}
		emit gamesChanged();
	}
	return writeFile(filePath(), ProfileFile{ _records, _rating });
}

void GameLibrary::scheduleSave()
{
	_dirty = true;
	if (_saveQueued)
		return;
	_saveQueued = true;
	QTimer::singleShot(0, this, [this]() {
		_saveQueued = false;
		flush();
	});
}

void GameLibrary::flush()
{
	if (_dirty)
		save();
}

QStringList GameLibrary::profileIds() const
{
	QStringList ids;
	if (_userManager)
		for (const auto& u : _userManager->users())
			ids.append(u.toMap().value(QStringLiteral("id")).toString());
	return ids;
}

int GameLibrary::finishedBeyond(int keep) const
{
	keep = std::max(1, keep);
	auto beyond = [keep](const QList<GameRecord>& records) {
		const int finished = (int)std::count_if(records.begin(), records.end(), [](const GameRecord& r) { return r.finished; });
		return std::max(0, finished - keep);
	};
	int total = beyond(_records);
	for (const auto& id : profileIds()) {
		if (id == _userId)
			continue;
		ProfileFile file;
		if (readFile(filePathOf(id), file) == FileState::Read)
			total += beyond(file.records);
	}
	return total;
}

void GameLibrary::trimOtherProfiles()
{
	for (const auto& id : profileIds()) {
		if (id == _userId)
			continue;
		ProfileFile file; // its rating stays as it is
		if (readFile(filePathOf(id), file) == FileState::Read && trimRecords(file.records, _keepCount))
			writeFile(filePathOf(id), file);
	}
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
	if (trimRecords(_records, _keepCount)) {
		save();
		emit gamesChanged();
	}
	trimOtherProfiles();
}

QString GameLibrary::put(GameRecord record)
{
	if (_userId.isEmpty())
		return {};
	if (record.id.isEmpty())
		record.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	if (!record.startedAt.isValid())
		record.startedAt = QDateTime::currentDateTime();
	if (!record.analysis.isEmpty() && !GameAnalysis::fromJson(record.analysis).fits(record.moves))
		record.analysis = {}; // the moves changed since (a takeback, the game went on)
	rate(record);
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == record.id; });
	if (it != _records.end())
		*it = record;
	else
		_records.prepend(record);
	std::stable_sort(_records.begin(), _records.end(), newerFirst);
	trim();
	scheduleSave();
	emit gamesChanged();
	return record.id;
}

// Once per result: a rated game keeps its change. Only the game on the board is put again after its
// result, and only a takeback un-finishes it (which also makes it unrateable: undoCount), so giving the
// change back right then restores the rating exactly.
void GameLibrary::rate(GameRecord& record)
{
	if (_unreadable) {
		// The rating isn't known while games.json can't be read (it shows the start rating): a change
		// worked out from that would be wrong. Rated when the file is read again (save()).
		if (!record.rated && record.rateable() && !_rateLater.contains(record.id))
			_rateLater.append(record.id);
		else if (!record.finished)
			_rateLater.removeAll(record.id);
		return;
	}
	if (record.rated && !record.finished) {
		_rating.revert(record.ratingChange);
		record.rated = false;
		record.ratingBefore = record.ratingChange = 0;
		emit ratingChanged();
	} else if (!record.rated && record.rateable()) {
		record.ratingBefore = _rating.value();
		record.ratingChange = _rating.changeFor(Chess::EngineLevel(record.level).DisplayElo(), record.playerScore());
		record.rated = true;
		_rating.apply(record.ratingChange);
		emit ratingChanged();
	}
}

bool GameLibrary::remove(const QString& id)
{
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
	if (it == _records.end())
		return false;
	_records.erase(it);
	scheduleSave();
	emit gamesChanged();
	return true;
}

bool GameLibrary::setAnalysis(const QString& id, const QStringList& moves, const GameAnalysis& analysis)
{
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
	if (it == _records.end() || it->moves != moves || !analysis.fits(moves))
		return false;
	it->analysis = analysis.toJson();
	scheduleSave();
	emit gamesChanged();
	return true;
}

// The oldest finished games beyond keepCount go (by when they finished); unfinished ones stay.
bool GameLibrary::trimRecords(QList<GameRecord>& records, int keep)
{
	QList<const GameRecord*> finished;
	for (const auto& r : records)
		if (r.finished)
			finished.append(&r);
	if (finished.size() <= keep)
		return false;
	std::sort(finished.begin(), finished.end(), [](const GameRecord* a, const GameRecord* b) {
		const QDateTime da = a->finishedAt.isValid() ? a->finishedAt : a->startedAt;
		const QDateTime db = b->finishedAt.isValid() ? b->finishedAt : b->startedAt;
		return da < db;
	});
	QStringList drop;
	for (int i = 0; i < finished.size() - keep; ++i)
		drop.append(finished[i]->id);
	records.erase(std::remove_if(records.begin(), records.end(),
		[&](const GameRecord& r) { return drop.contains(r.id); }), records.end());
	return true;
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
		// The autosave of a Lichess game (or of a version that kept others): those versions never
		// offered it, and the game is Lichess's. Not one for this library.
		if (s.autosave && !s.info.isSinglePlayer) {
			QFile::remove(s.path);
			continue;
		}
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
			// Kept aside, not lost: a later version may read it.
			qWarning() << "GameLibrary: can't migrate" << s.path << e.what();
			keepAside(s.path);
			continue;
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
