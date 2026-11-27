#include "stdafx.h"
#include "gamelibrary.h"

#include "AppConfig.h"
#include "EngineLevel.h"
#include "Game.h"
#include "gameanalysis.h"
#include "gamesignature.h"
#include "UserManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
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

// ── LichessGameInfo ──────────────────────────────────────────────────────────

LichessGameInfo::LichessGameInfo(const QString& gameId, bool rated, const QString& speed, int daysPerTurn,
                                 const OnlinePlayer& white, const OnlinePlayer& black)
	: _gameId(gameId), _rated(rated), _speed(speed), _daysPerTurn(daysPerTurn), _white(white), _black(black)
{
}

QString LichessGameInfo::url() const
{
	return _gameId.isEmpty() ? QString() : QStringLiteral("https://lichess.org/") + _gameId;
}

QJsonObject LichessGameInfo::toJson() const
{
	QJsonObject o{
		{ QStringLiteral("gameId"), _gameId },
		{ QStringLiteral("rated"),  _rated },
		{ QStringLiteral("white"),  _white.toJson() },
		{ QStringLiteral("black"),  _black.toJson() },
	};
	if (!_speed.isEmpty())
		o[QStringLiteral("speed")] = _speed;
	if (_daysPerTurn > 0)
		o[QStringLiteral("daysPerTurn")] = _daysPerTurn;
	return o;
}

LichessGameInfo LichessGameInfo::fromJson(const QJsonObject& o)
{
	if (o.value(QStringLiteral("gameId")).toString().isEmpty())
		return {};
	return LichessGameInfo(o.value(QStringLiteral("gameId")).toString(), o.value(QStringLiteral("rated")).toBool(),
		o.value(QStringLiteral("speed")).toString(), o.value(QStringLiteral("daysPerTurn")).toInt(),
		OnlinePlayer::fromJson(o.value(QStringLiteral("white")).toObject()),
		OnlinePlayer::fromJson(o.value(QStringLiteral("black")).toObject()));
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
		if (ratedGames > 0)
			o[QStringLiteral("ratedGames")] = ratedGames;
	}
	if (!lichess.isEmpty())
		o[QStringLiteral("lichess")]      = lichess.toJson();
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
	r.ratedGames       = r.rated ? o.value(QStringLiteral("ratedGames")).toInt(0) : 0;
	r.lichess          = LichessGameInfo::fromJson(o.value(QStringLiteral("lichess")).toObject());
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
		// LI-14: a kept Lichess game's own facts ("" / false / 0 for other games).
		{ QStringLiteral("lichessUrl"),       lichess.url() },
		{ QStringLiteral("lichessRated"),     lichess.rated() },
		{ QStringLiteral("lichessSpeed"),     lichess.speed() },
		{ QStringLiteral("lichessDays"),      lichess.daysPerTurn() },
		{ QStringLiteral("opponentTitle"),    lichess.player(!playerPlaysWhite).title() },
		{ QStringLiteral("opponentRating"),   lichess.player(!playerPlaysWhite).ratingText() },
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

// ── The files (BL-20) ────────────────────────────────────────────────────────

QString GameLibrary::gamesDirOf(const QString& userId)
{
	return AppConfig::gamesDirPath(userId);
}

QString GameLibrary::gameFilePathOf(const QString& userId, const QString& gameId)
{
	return gamesDirOf(userId) + QLatin1Char('/') + gameId + QStringLiteral(".json");
}

QString GameLibrary::rejectedDirOf(const QString& userId)
{
	return gamesDirOf(userId) + QStringLiteral("/rejected");
}

QString GameLibrary::ratingFilePathOf(const QString& userId)
{
	return AppConfig::ratingFilePath(userId);
}

QString GameLibrary::legacyFilePathOf(const QString& userId)
{
	return AppConfig::gamesFilePath(userId);
}

bool GameLibrary::isFileId(const QString& id)
{
	static const QRegularExpression fileId(QRegularExpression::anchoredPattern(QStringLiteral("[A-Za-z0-9_-]{1,64}")));
	return fileId.match(id).hasMatch();
}

QString GameLibrary::fileIdOf(const QString& id)
{
	if (isFileId(id))
		return id;
	static const QUuid ns(QStringLiteral("{6f0c6c55-0f4e-4d0b-9a59-3b2f3c2a1b20}")); // never change it
	return QUuid::createUuidV5(ns, id).toString(QUuid::WithoutBraces);
}

bool GameLibrary::hasSignedFiles(const QString& userId)
{
	if (userId.isEmpty())
		return false;
	const QString dir = gamesDirOf(userId);
	return QFileInfo::exists(ratingFilePathOf(userId))
		|| QFileInfo::exists(legacyFilePathOf(userId) + QStringLiteral(".migrated"))
		|| QFileInfo::exists(rejectedDirOf(userId))
		|| !QDir(dir).entryList({ QStringLiteral("*.json") }, QDir::Files).isEmpty();
}

// Where a game is listed: when it began; a Lichess game when it ended here (LI-14: a correspondence game can
// have begun weeks before, and the one just finished belongs at the top). Its startedAt stays Lichess's, for
// the PGN's Date.
static QDateTime listedAt(const GameRecord& r)
{
	return r.isLichess() && r.finishedAt.isValid() ? r.finishedAt : r.startedAt;
}

static bool newerFirst(const GameRecord& a, const GameRecord& b)
{
	return listedAt(a) > listedAt(b);
}

QByteArray GameLibrary::gameFileContent(const GameRecord& record)
{
	const QJsonObject json = record.toJson();
	const QJsonObject root{
		{ QStringLiteral("record"), json },
		{ QStringLiteral("sig"),    GameSignature::sign(GameSignature::Kind::Game, json) },
	};
	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QString GameLibrary::describe(ReadFailure why)
{
	switch (why) {
	case ReadFailure::None:         return {};
	case ReadFailure::CantOpen:     return QStringLiteral("can't be opened");
	case ReadFailure::NotJson:      return QStringLiteral("not JSON");
	case ReadFailure::BadSignature: return QStringLiteral("its signature doesn't fit");
	case ReadFailure::OtherId:      return QStringLiteral("another game's id");
	}
	return {};
}

std::optional<GameRecord> GameLibrary::readGameFile(const QString& path, ReadFailure* why, bool* resign)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		if (why)
			*why = ReadFailure::CantOpen;
		return std::nullopt;
	}
	return readGameContent(file.readAll(), QFileInfo(path).completeBaseName(), why, resign);
}

std::optional<GameRecord> GameLibrary::readGameContent(const QByteArray& content, const QString& fileId,
                                                       ReadFailure* why, bool* resign)
{
	auto fail = [why](ReadFailure reason) -> std::optional<GameRecord> {
		if (why)
			*why = reason;
		return std::nullopt;
	};
	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(content, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return fail(ReadFailure::NotJson);
	const QJsonObject json = doc.object().value(QStringLiteral("record")).toObject();
	const QString sig = doc.object().value(QStringLiteral("sig")).toString();
	if (json.isEmpty() || !GameSignature::verify(GameSignature::Kind::Game, json, sig))
		return fail(ReadFailure::BadSignature);
	GameRecord record = GameRecord::fromJson(json);
	if (record.id != fileId)
		return fail(ReadFailure::OtherId); // a game copied under another name
	if (why)
		*why = ReadFailure::None;
	if (resign)
		*resign = !GameSignature::isCurrent(sig);
	return record;
}

// Moved, not deleted: a damaged one can be looked at; an edited one isn't lost.
static void setAsideFile(const QString& path, const QString& rejectedDir, const QString& why)
{
	QDir().mkpath(rejectedDir);
	const QFileInfo info(path);
	QString target = rejectedDir + QLatin1Char('/') + info.fileName();
	if (QFileInfo::exists(target))
		target = rejectedDir + QLatin1Char('/') + info.completeBaseName()
			+ QDateTime::currentDateTime().toString(QStringLiteral("-yyyyMMdd-HHmmss.")) + info.suffix();
	if (!QFile::rename(path, target))
		qWarning() << "GameLibrary: can't set aside" << path;
	qWarning() << "GameLibrary:" << info.fileName() << "set aside:" << why;
}

void GameLibrary::readProfile(const QString& userId, ProfileFile& profileFile, bool setAside)
{
	profileFile = {};
	const QString dir = gamesDirOf(userId);
	for (const QFileInfo& info : QDir(dir).entryInfoList({ QStringLiteral("*.json") }, QDir::Files)) {
		ReadFailure why = ReadFailure::None;
		bool resign = false;
		const auto record = readGameFile(info.filePath(), &why, &resign);
		if (!record) {
			if (why == ReadFailure::CantOpen) {
				qWarning() << "GameLibrary: can't read" << info.filePath() << "now; left as it is";
				continue; // a lock: it is there next time
			}
			++profileFile.setAside;
			if (setAside)
				setAsideFile(info.filePath(), rejectedDirOf(userId), describe(why));
			continue;
		}
		if (record->moves.isEmpty())
			continue; // nothing to keep
		profileFile.records.append(*record);
		if (resign)
			profileFile.resign.append(record->id);
	}
	std::stable_sort(profileFile.records.begin(), profileFile.records.end(), newerFirst);

	const QString ratingPath = ratingFilePathOf(userId);
	if (!QFileInfo::exists(ratingPath)) {
		profileFile.ratingState = RatingState::Missing;
	} else if (const auto rating = readRatingFile(ratingPath)) {
		profileFile.rating = *rating;
		profileFile.ratingState = RatingState::Verified;
	} else {
		profileFile.ratingState = RatingState::Failed;
	}
}

QList<GameRecord> GameLibrary::keptRecordsOf(const QString& userId)
{
	ProfileFile file;
	readProfile(userId, file, false);
	return file.records;
}

static QDateTime endedAt(const GameRecord& r)
{
	return r.finishedAt.isValid() ? r.finishedAt : r.startedAt;
}

std::optional<LocalRating> GameLibrary::ratingFromGames(const QList<GameRecord>& records)
{
	QList<const GameRecord*> rated;
	for (const auto& r : records)
		if (r.rated)
			rated.append(&r);
	std::stable_sort(rated.begin(), rated.end(), [](const GameRecord* a, const GameRecord* b) { return endedAt(*a) < endedAt(*b); });
	std::optional<LocalRating> newest;
	for (int i = 0; i < rated.size(); ++i) {
		// The most rated games after it is the newest (the time only orders records without ratedGames).
		const int games = rated[i]->ratedGames > 0 ? rated[i]->ratedGames : i + 1;
		if (!newest || games >= newest->games())
			newest = LocalRating(rated[i]->ratingBefore + rated[i]->ratingChange, games);
	}
	return newest;
}

QByteArray GameLibrary::ratingFileContent(const LocalRating& rating)
{
	const QJsonObject json = rating.toJson();
	const QJsonObject root{
		{ QStringLiteral("version"), RatingFormatVersion },
		{ QStringLiteral("rating"),  json },
		{ QStringLiteral("sig"),     GameSignature::sign(GameSignature::Kind::Rating, json) },
	};
	return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

// rating.json's content, checked: nullopt when it isn't JSON or its signature doesn't fit.
static std::optional<LocalRating> verifiedRating(const QByteArray& content)
{
	const QJsonObject root = QJsonDocument::fromJson(content).object();
	const QJsonObject json = root.value(QStringLiteral("rating")).toObject();
	if (json.isEmpty()
		|| !GameSignature::verify(GameSignature::Kind::Rating, json, root.value(QStringLiteral("sig")).toString()))
		return std::nullopt;
	return LocalRating::fromJson(json);
}

std::optional<LocalRating> GameLibrary::readRatingFile(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return std::nullopt;
	return verifiedRating(file.readAll());
}

LocalRating GameLibrary::continued(LocalRating rating, const QList<GameRecord>& records)
{
	for (bool found = true; found;) {
		found = false;
		for (const auto& r : records) {
			if (r.rated && r.ratedGames == rating.games() + 1 && r.ratingBefore == rating.value()) {
				rating.apply(r.ratingChange);
				found = true;
				break;
			}
		}
	}
	return rating;
}

void GameLibrary::recoverRating(RatingState state)
{
	if (state == RatingState::Verified) {
		// rating.json is written after its game (two writes): a crash between them leaves it behind. Only
		// the games that go on from it count, not just any newer one (review of #187-#190: a rated game
		// of another profile, copied here, verifies too).
		const LocalRating caughtUp = continued(_rating, _records);
		if (caughtUp.games() > _rating.games()) {
			qWarning() << "GameLibrary: rating.json is behind the rated games that continue it; caught up";
			_rating = caughtUp;
			_ratingChanged = true;
		}
		return;
	}
	const auto fromGames = ratingFromGames(_records);
	switch (state) {
	case RatingState::Verified:
		return;
	case RatingState::Missing:
		if (!fromGames)
			return; // nothing rated yet
		break;
	case RatingState::Failed:
		setAsideFile(ratingFilePathOf(_userId), rejectedDirOf(_userId), QStringLiteral("its signature doesn't fit"));
		break;
	}
	_rating = fromGames.value_or(LocalRating());
	_ratingChanged = true;
	_ratingRestored = true;
	qWarning() << "GameLibrary: the rating of" << _userId << "restored:" << _rating.value() << _rating.games() << "games";
}

bool GameLibrary::writeGameFile(const QString& userId, const GameRecord& record)
{
	return AppConfig::writeFileAtomically(gameFilePathOf(userId, record.id), gameFileContent(record));
}

bool GameLibrary::writeRatingFile(const QString& userId, const LocalRating& rating)
{
	return AppConfig::writeFileAtomically(ratingFilePathOf(userId), ratingFileContent(rating));
}

// games.json of BL-19: every record and the rating in one file.
GameLibrary::FileState GameLibrary::readLegacyFile(const QString& path, ProfileFile& profileFile)
{
	auto& records = profileFile.records;
	records.clear();
	profileFile.rating = {};
	if (!QFileInfo::exists(path))
		return FileState::Missing;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return FileState::Unreadable;
	return parseLegacy(file.readAll(), profileFile);
}

GameLibrary::FileState GameLibrary::parseLegacy(const QByteArray& content, ProfileFile& profileFile)
{
	auto& records = profileFile.records;
	records.clear();
	profileFile.rating = {};
	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(content, &err);
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

GameLibrary::ImportCheck GameLibrary::checkImport(QMap<QString, QByteArray>& files)
{
	ImportCheck check;
	const QString games = QStringLiteral("games/");
	const QString rejected = QStringLiteral("games/rejected/");
	const QString legacyName = QStringLiteral("games.json");
	const QString ratingName = QStringLiteral("rating.json");
	QList<GameRecord> verified;
	for (const QString& name : files.keys()) { // a copy: failing ones move
		if (!name.startsWith(games) || name.startsWith(rejected))
			continue;
		ReadFailure why = ReadFailure::None;
		const QString fileName = name.mid(games.size());
		if (const auto record = readGameContent(files.value(name), QFileInfo(fileName).completeBaseName(), &why)) {
			verified.append(*record);
			continue;
		}
		qWarning() << "GameLibrary: the imported" << name << "set aside:" << describe(why);
		files.insert(rejected + fileName, files.take(name));
		++check.setAside;
	}
	if (files.contains(legacyName)) {
		// An export from before BL-20: nothing in it was signed, so its games come in without their
		// rating fields and no rating comes with it (§2.7). One that can't be read is written as it is:
		// the profile keeps it aside as .damaged when it loads.
		check.unrated = true;
		files.remove(ratingName);
		ProfileFile legacy;
		if (parseLegacy(files.value(legacyName), legacy) != FileState::Read)
			return check;
		for (GameRecord r : legacy.records) {
			r.rated = false;
			r.ratingBefore = r.ratingChange = r.ratedGames = 0;
			r.id = fileIdOf(r.id);
			const QString name = games + r.id + QStringLiteral(".json");
			if (!files.contains(name))
				files.insert(name, gameFileContent(r));
		}
		files.remove(legacyName);
		return check;
	}
	if (files.contains(ratingName)) {
		if (!verifiedRating(files.value(ratingName))) {
			qWarning() << "GameLibrary: the imported rating.json set aside: its signature doesn't fit";
			files.insert(rejected + ratingName, files.take(ratingName));
			if (const auto rating = ratingFromGames(verified))
				files.insert(ratingName, ratingFileContent(*rating));
			check.ratingRestored = true;
		}
	}
	return check;
}

// A damaged file is kept aside, not overwritten or deleted: the owner may want to look at it.
static void keepAside(const QString& path)
{
	const QString aside = path + QStringLiteral(".damaged");
	QFile::remove(aside);
	QFile::rename(path, aside);
	qWarning() << "GameLibrary:" << path << "is damaged, kept as" << aside;
}

// The profile's games.json (BL-19), once: its games become files of their own, signed (trusted: they
// were never signed), its rating rating.json when there is none yet (games rated while it couldn't be
// read moved this one). Written now, then renamed games.json.migrated (kept, never read again).
GameLibrary::FileState GameLibrary::migrateLegacyFile()
{
	const QString path = legacyFilePathOf(_userId);
	ProfileFile legacy;
	const FileState state = readLegacyFile(path, legacy);
	if (state == FileState::Damaged)
		keepAside(path);
	if (state != FileState::Read)
		return state;
	// BL-20: each rated record gets the rated games after it, counted back from the rating's along the
	// rated records, oldest first (the newest has them all).
	QList<GameRecord*> rated;
	for (auto& r : legacy.records)
		if (r.rated)
			rated.append(&r);
	std::stable_sort(rated.begin(), rated.end(), [](const GameRecord* a, const GameRecord* b) { return endedAt(*a) < endedAt(*b); });
	const int n = (int)rated.size();
	for (int i = 0; i < n; ++i)
		if (rated[i]->ratedGames == 0)
			rated[i]->ratedGames = std::max(i + 1, legacy.rating.games() - (n - 1 - i));
	int added = 0;
	for (GameRecord r : legacy.records) {
		r.id = fileIdOf(r.id);
		if (find(r.id))
			continue; // already a file of its own (a migration cut short)
		_records.append(r);
		_changed.insert(r.id);
		++added;
	}
	std::stable_sort(_records.begin(), _records.end(), newerFirst);
	if (!QFileInfo::exists(ratingFilePathOf(_userId)) && legacy.rating.games() > 0) {
		_rating = legacy.rating;
		_ratingChanged = true;
	}
	_legacyPending = false;
	_dirty = true;
	if (!save())
		return FileState::Unreadable; // not written: games.json stays, tried again
	const QString migrated = path + QStringLiteral(".migrated");
	QFile::remove(migrated);
	QFile::rename(path, migrated);
	qInfo() << "GameLibrary: migrated" << added << "game(s) of" << path << "into" << gamesDir();
	return FileState::Read;
}

void GameLibrary::reload()
{
	flush(); // the profile being left: its last change
	const QString userId = _userManager && _userManager->isLoggedIn() ? _userManager->activeUserId() : QString();
	_userId = userId;
	_legacyPending = false;
	_records.clear();
	_rating = {};
	_setAside = 0;
	_changed.clear();
	_removed.clear();
	_rateLater.clear();
	_ratingChanged = false;
	_ratingRestored = false;
	if (!_userId.isEmpty()) {
		// games.json is trusted once, as this PC's own from before BL-20: never beside files of BL-20
		// (review of #187-#190: a games.json written by hand would sign whatever it says). Set aside.
		const bool legacyTrusted = !hasSignedFiles(_userId);
		ProfileFile file;
		readProfile(_userId, file, true);
		if (!legacyTrusted && QFileInfo::exists(legacyFilePathOf(_userId))) {
			setAsideFile(legacyFilePathOf(_userId), rejectedDirOf(_userId), QStringLiteral("beside signed files: not trusted"));
			++file.setAside;
		}
		_records = file.records;
		_rating = file.rating;
		_setAside = file.setAside;
		for (const auto& id : file.resign)
			_changed.insert(id);
		if (!_changed.isEmpty())
			scheduleSave();
		RatingState ratingState = file.ratingState;
		switch (legacyTrusted ? migrateLegacyFile() : FileState::Damaged) {
		case FileState::Unreadable:
			// There, but not to be read now (another program holds it): its games aren't shown, and it
			// isn't written over; every write tries it again.
			_legacyPending = true;
			qWarning() << "GameLibrary: can't read" << legacyFilePathOf(_userId) << "now; it is kept as it is";
			break;
		case FileState::Missing:
			migrateOldSaves();
			break;
		case FileState::Read:
			if (ratingState == RatingState::Missing)
				ratingState = RatingState::Verified; // games.json's rating, trusted once as its games
			break;
		case FileState::Damaged:
			break;
		}
		if (!_legacyPending) { // else the rating isn't known yet: games.json's games may move it
			recoverRating(ratingState);
			if (_ratingChanged)
				scheduleSave();
		}
	}
	emit gamesChanged();
	emit ratingChanged();
	if (_setAside > 0)
		emit gamesSetAside(_setAside);
	if (_ratingRestored)
		emit ratingRestoredOnLoad();
}

bool GameLibrary::save()
{
	_dirty = false;
	if (_userId.isEmpty())
		return false;
	if (_legacyPending && QFileInfo::exists(legacyFilePathOf(_userId))) {
		// The games of games.json and the ones kept since go in together; then the rating is known, and
		// the games that finished meanwhile are rated.
		_legacyPending = false;
		switch (migrateLegacyFile()) {
		case FileState::Unreadable:
			_legacyPending = true;
			qWarning() << "GameLibrary: still can't read" << legacyFilePathOf(_userId);
			break;
		default:
			for (const QString& id : std::exchange(_rateLater, {})) {
				const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
				if (it != _records.end()) {
					rate(*it);
					_changed.insert(id);
				}
			}
			trim();
			emit gamesChanged();
			break;
		}
	}
	bool ok = true;
	bool removeLater = false;
	for (const QString& id : std::exchange(_removed, {})) {
		const QString path = gameFilePathOf(_userId, id);
		if (!QFile::remove(path) && QFileInfo::exists(path)) {
			// Held by another program (an antivirus): tried again at the next write, else the game comes
			// back at the next start (review of #187-#190).
			qWarning() << "GameLibrary: can't remove" << path << "now; tried again";
			_removed.insert(id);
			removeLater = true;
		}
	}
	const QSet<QString> changed = std::exchange(_changed, {});
	for (const QString& id : changed) {
		const auto r = find(id);
		if (r && !writeGameFile(_userId, *r)) {
			_changed.insert(id); // tried again with the next change
			ok = false;
		}
	}
	// The rating is written once it is known (not while games.json waits: its games may move it).
	if (_ratingChanged && !_legacyPending) {
		if (writeRatingFile(_userId, _rating))
			_ratingChanged = false;
		else
			ok = false;
	}
	if (!ok || removeLater || _legacyPending)
		_dirty = true; // tried again at the next write (games.json: until it can be read)
	return ok;
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
		readProfile(id, file, false);
		total += beyond(file.records);
	}
	return total;
}

void GameLibrary::trimOtherProfiles()
{
	for (const auto& id : profileIds()) {
		if (id == _userId)
			continue;
		ProfileFile file;
		readProfile(id, file, false); // a failing file stays where it is: that profile sets it aside when it loads
		for (const QString& gameId : trimRecords(file.records, _keepCount))
			QFile::remove(gameFilePathOf(id, gameId));
	}
}

void GameLibrary::trim()
{
	for (const QString& id : trimRecords(_records, _keepCount)) {
		_changed.remove(id);
		_removed.insert(id);
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
	const int before = (int)_records.size();
	trim();
	if ((int)_records.size() != before) {
		save();
		emit gamesChanged();
	}
	trimOtherProfiles();
}

QString GameLibrary::put(GameRecord record)
{
	if (_userId.isEmpty())
		return {};
	if (record.id.isEmpty() || !isFileId(record.id))
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
	_changed.insert(record.id);
	_removed.remove(record.id);
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
	if (_legacyPending) {
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
		record.ratingBefore = record.ratingChange = record.ratedGames = 0;
		_ratingChanged = true;
		emit ratingChanged();
	} else if (!record.rated && record.rateable()) {
		record.ratingBefore = _rating.value();
		record.ratingChange = _rating.changeFor(Chess::EngineLevel(record.level).DisplayElo(), record.playerScore());
		record.rated = true;
		_rating.apply(record.ratingChange);
		record.ratedGames = _rating.games();
		_ratingChanged = true;
		emit ratingChanged();
	}
}

bool GameLibrary::remove(const QString& id)
{
	const auto it = std::find_if(_records.begin(), _records.end(), [&](const GameRecord& r) { return r.id == id; });
	if (it == _records.end())
		return false;
	_records.erase(it);
	_changed.remove(id);
	_removed.insert(id);
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
	_changed.insert(id);
	scheduleSave();
	emit gamesChanged();
	return true;
}

// The oldest finished games beyond keepCount go (by when they finished); unfinished ones stay.
QStringList GameLibrary::trimRecords(QList<GameRecord>& records, int keep)
{
	QList<const GameRecord*> finished;
	for (const auto& r : records)
		if (r.finished)
			finished.append(&r);
	if (finished.size() <= keep)
		return {};
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
	return drop;
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
				_changed.insert(r.id);
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
	qInfo() << "GameLibrary: migrated" << _records.size() << "saved game(s) of the profile into" << gamesDir();
}
