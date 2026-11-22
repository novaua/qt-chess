#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <CppUnitTest.h>
#include "AppConfig.h"
#include "Game.h"
#include "UserManager.h"
#include "gamelibrary.h"
#include "gamesignature.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// The games library (BL-19) on a temporary data folder: records kept per profile, one signed file per
// game (BL-20), the cap on finished games, games that fail their check set aside, the migration of
// games.json (BL-19) and of the two save slots of earlier versions.
namespace GameLibraryTests
{
	static QByteArray readFile(const QString& path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static GameRecord makeRecord(const QString& kind, bool finished, const QStringList& moves,
		const QDateTime& started = QDateTime::currentDateTime())
	{
		GameRecord r;
		r.kind = kind;
		r.finished = finished;
		r.startedAt = started;
		if (finished) {
			r.finishedAt = started.addSecs(600);
			r.result = QStringLiteral("1-0");
			r.resultText = QStringLiteral("White wins by checkmate");
		}
		r.opponentName = kind == QLatin1String("computer") ? QStringLiteral("Robot") : QStringLiteral("Guest");
		r.level = kind == QLatin1String("computer") ? 3 : 0;
		r.moves = moves;
		r.fen = QStringLiteral("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
		return r;
	}

	// A profile's games.json as BL-19 wrote it (one file, unsigned, the rating inside).
	static QByteArray legacyContent(const QList<GameRecord>& records, int rating = 1200, int ratedGames = 0)
	{
		QJsonArray games;
		for (GameRecord r : records) {
			if (r.id.isEmpty())
				r.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
			games.append(r.toJson());
		}
		QJsonObject root{ { "version", 1 }, { "games", games } };
		if (ratedGames > 0)
			root["rating"] = QJsonObject{ { "value", rating }, { "games", ratedGames } };
		return QJsonDocument(root).toJson(QJsonDocument::Compact);
	}

	static void writeFile(const QString& path, const QByteArray& data)
	{
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile f(path);
		Assert::IsTrue(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
		f.write(data);
	}

	static int jsonFilesIn(const QString& dir)
	{
		return (int)QDir(dir).entryList({ QStringLiteral("*.json") }, QDir::Files).size();
	}

	TEST_CLASS(GameLibraryTest)
	{
		std::unique_ptr<QTemporaryDir> _dir;
		std::unique_ptr<UserManager> _users;
		std::unique_ptr<GameLibrary> _library;

		void makeLibrary()
		{
			_users = std::make_unique<UserManager>(); // loads the profiles
			_library = std::make_unique<GameLibrary>();
			_library->setUserManager(_users.get());
		}

		QString addUser(const QString& name)
		{
			_users->createUser(name, QStringLiteral("wizard"));
			return _users->activeUserId();
		}

	public:
		TEST_METHOD_INITIALIZE(SetUp)
		{
			_dir = std::make_unique<QTemporaryDir>();
			AppConfig::setAppDataDir(_dir->path());
			makeLibrary();
		}

		TEST_METHOD_CLEANUP(TearDown)
		{
			_library.reset();
			_users.reset();
			AppConfig::setAppDataDir({});
			_dir.reset();
		}

		TEST_METHOD(NothingIsKeptWithoutAProfile)
		{
			Assert::AreEqual(QString(), _library->put(makeRecord("local", false, { "e2e4" })));
			Assert::AreEqual(0, _library->count());
		}

		TEST_METHOD(ARecordRoundTripsThroughTheFile)
		{
			const QString user = addUser("Alice");
			GameRecord r = makeRecord("computer", false, { "e2e4", "e7e5", "g1f3" });
			r.playerPlaysWhite = false;
			r.opponentUserId = QString();
			r.clockLimitSec = 300; r.clockIncSec = 2; r.whiteMs = 123456; r.blackMs = 234567;
			r.undoCount = 2;
			r.forkOfId = QStringLiteral("abc"); r.forkPly = 7;
			r.playerAvatar = QStringLiteral("owl"); r.opponentAvatar = QStringLiteral("robot");
			QSignalSpy changed(_library.get(), &GameLibrary::gamesChanged);
			const QString id = _library->put(r);
			Assert::IsFalse(id.isEmpty(), L"an id is given");
			Assert::AreEqual(1, (int)changed.count());
			Assert::IsFalse(QFile::exists(GameLibrary::gameFilePathOf(user, id)), L"written once the event is done, not at once");
			QCoreApplication::processEvents();
			Assert::IsTrue(QFile::exists(GameLibrary::gameFilePathOf(user, id)), L"its file written");

			makeLibrary(); // read again from the file (the profile is still logged in)
			Assert::AreEqual(1, _library->count());
			const auto kept = _library->find(id);
			Assert::IsTrue(kept.has_value());
			Assert::AreEqual(QStringLiteral("computer"), kept->kind);
			Assert::IsFalse(kept->finished);
			Assert::IsFalse(kept->playerPlaysWhite);
			Assert::AreEqual(3, kept->level);
			Assert::AreEqual(QStringLiteral("Robot"), kept->opponentName);
			Assert::AreEqual(300, kept->clockLimitSec);
			Assert::AreEqual(2, kept->clockIncSec);
			Assert::AreEqual((qint64)123456, kept->whiteMs);
			Assert::AreEqual((qint64)234567, kept->blackMs);
			Assert::AreEqual(QStringLiteral("e2e4 e7e5 g1f3"), kept->moves.join(' '));
			Assert::AreEqual(r.fen, kept->fen);
			Assert::AreEqual(2, kept->undoCount);
			Assert::AreEqual(QStringLiteral("abc"), kept->forkOfId);
			Assert::AreEqual(7, kept->forkPly);
			Assert::AreEqual(QStringLiteral("owl"), kept->playerAvatar);
			Assert::AreEqual(r.startedAt.toSecsSinceEpoch(), kept->startedAt.toSecsSinceEpoch());
			Assert::IsTrue(kept->playerToMove(), L"after three plies Black moves, the player is Black: the player's turn");
			Assert::AreEqual(QStringLiteral("g1f3"), kept->lastMove());

			const QVariantMap v = kept->toVariant();
			Assert::AreEqual(3, v.value("plyCount").toInt());
			Assert::AreEqual(QStringLiteral("g1f3"), v.value("lastMove").toString());
			Assert::IsFalse(v.value("analysed").toBool());
		}

		// LI-14: a finished Lichess game, its id from the Lichess id, both players (as gameFull gives them, with
		// the opponent's flair) through the signed file. Never rated; it counts under the same cap.
		TEST_METHOD(ALichessGame_KeptWithItsPlayers_NeverRated)
		{
			const QString user = addUser("Alice");
			const QDateTime t0 = QDateTime::currentDateTime().addDays(-1);
			GameRecord r = makeRecord("lichess", true, { "e2e4", "e7e5" }, t0);
			r.id = GameRecord::lichessRecordId("abcd1234");
			r.opponentName = QStringLiteral("Magnus");
			r.clockLimitSec = 600;
			r.clockIncSec = 5;
			r.resultRecorded = true;
			const LichessPlayer me = LichessPlayer::fromJson(QJsonObject{
				{ "id", "alice" }, { "name", "Alice" }, { "rating", 1500 }, { "provisional", true } });
			LichessPlayer them = LichessPlayer::fromJson(QJsonObject{
				{ "id", "magnus" }, { "name", "Magnus" }, { "title", "GM" }, { "rating", 2850 } });
			them.setFlairUrl(QStringLiteral("https://lichess1.org/assets/flair/img/people.crown.webp"));
			r.lichess = LichessGameInfo("abcd1234", true, "rapid", 0, me, them);
			Assert::IsTrue(r.isLichess());
			Assert::IsFalse(r.rateable(), L"only robot games are rated here");
			Assert::IsTrue(GameLibrary::isFileId(r.id));
			Assert::AreEqual(QStringLiteral("lichess-abcd1234"), _library->put(r), L"the id is kept as given");
			_library->flush();

			makeLibrary(); // read again from the signed file
			const auto kept = _library->find(QStringLiteral("lichess-abcd1234"));
			Assert::IsTrue(kept.has_value());
			Assert::IsTrue(kept->isLichess());
			Assert::IsTrue(kept->finished);
			Assert::IsFalse(kept->rated);
			Assert::AreEqual(1200, _library->rating());
			Assert::AreEqual(0, _library->ratedGames());
			const LichessGameInfo& info = kept->lichess;
			Assert::AreEqual(QStringLiteral("abcd1234"), info.gameId());
			Assert::AreEqual(QStringLiteral("https://lichess.org/abcd1234"), info.url());
			Assert::IsTrue(info.rated());
			Assert::AreEqual(QStringLiteral("rapid"), info.speed());
			Assert::AreEqual(QStringLiteral("Alice"), info.player(true).name());
			Assert::AreEqual(QStringLiteral("alice"), info.player(true).id());
			Assert::AreEqual(QStringLiteral("1500?"), info.player(true).ratingText(), L"provisional");
			Assert::AreEqual(QStringLiteral("GM"), info.player(false).title());
			Assert::AreEqual(2850, info.player(false).rating());
			Assert::AreEqual(them.flairUrl(), info.player(false).flairUrl());
			Assert::AreEqual(QStringLiteral("e2e4 e7e5"), kept->moves.join(' '));

			// Lichess's AI keeps its level (its name is made from it), an anonymous player has none.
			const LichessPlayer ai = LichessPlayer::fromJson(LichessPlayer::fromJson(QJsonObject{ { "aiLevel", 3 } }).toJson());
			Assert::IsTrue(ai.isAi());
			Assert::AreEqual(QStringLiteral("Stockfish level 3"), ai.name());
			Assert::IsTrue(LichessPlayer().toJson().isEmpty());
			Assert::IsTrue(GameRecord::fromJson(makeRecord("local", true, { "e2e4" }).toJson()).lichess.isEmpty(),
				L"a local game has none");

			// One cap for every finished game: a newer robot game pushes the Lichess one out.
			_library->put(makeRecord("computer", true, { "e2e4" }, t0.addSecs(3600)));
			_library->setKeepCount(1);
			Assert::IsFalse(_library->find(QStringLiteral("lichess-abcd1234")).has_value());
			Assert::AreEqual(1, _library->count());
		}

		TEST_METHOD(PutWithTheSameIdReplaces)
		{
			addUser("Alice");
			const QString id = _library->put(makeRecord("local", false, { "e2e4" }));
			GameRecord r = *_library->find(id);
			r.moves << "e7e5";
			_library->put(r);
			Assert::AreEqual(1, _library->count());
			Assert::AreEqual(2, _library->find(id)->plyCount());
			GameRecord done = *_library->find(id);
			done.finished = true; done.result = "1/2-1/2";
			_library->put(done);
			Assert::AreEqual(0, _library->unfinishedCount());
			Assert::AreEqual(1, (int)_library->games().size());
		}

		TEST_METHOD(NewestFirstAndTheUnfinishedList)
		{
			addUser("Alice");
			const QDateTime t0 = QDateTime::currentDateTime().addDays(-3);
			const QString old = _library->put(makeRecord("local", true, { "e2e4" }, t0));
			const QString middle = _library->put(makeRecord("computer", false, { "d2d4" }, t0.addDays(1)));
			const QString newest = _library->put(makeRecord("local", false, { "c2c4" }, t0.addDays(2)));
			const auto& records = _library->records();
			Assert::AreEqual(newest, records[0].id);
			Assert::AreEqual(middle, records[1].id);
			Assert::AreEqual(old, records[2].id);
			const auto unfinished = _library->unfinished();
			Assert::AreEqual(2, (int)unfinished.size());
			Assert::AreEqual(newest, unfinished[0].id);
			Assert::AreEqual(2, (int)_library->unfinishedGames().size());
			Assert::AreEqual(middle, _library->unfinishedGames()[1].toMap().value("id").toString());
		}

		TEST_METHOD(RemoveDeletesOnlyThatGame)
		{
			addUser("Alice");
			const QString a = _library->put(makeRecord("local", true, { "e2e4" }));
			const QString b = _library->put(makeRecord("local", false, { "d2d4" }));
			Assert::IsTrue(_library->remove(a));
			Assert::IsFalse(_library->remove(a), L"gone already");
			Assert::AreEqual(1, _library->count());
			Assert::IsTrue(_library->find(b).has_value());
			makeLibrary();
			Assert::AreEqual(1, _library->count(), L"the file says so too");
		}

		// keepCount finished games stay, the oldest finished (by when they finished) go; unfinished
		// games don't count and never go on their own.
		TEST_METHOD(TheCapDropsTheOldestFinishedGames)
		{
			addUser("Alice");
			_library->setKeepCount(3);
			const QDateTime t0 = QDateTime::currentDateTime().addDays(-10);
			QStringList finished;
			for (int i = 0; i < 5; ++i)
				finished << _library->put(makeRecord("computer", true, { "e2e4" }, t0.addDays(i)));
			const QString unfinished = _library->put(makeRecord("local", false, { "d2d4" }, t0));
			Assert::AreEqual(4, _library->count(), L"3 finished + 1 unfinished");
			Assert::IsFalse(_library->find(finished[0]).has_value(), L"the oldest went");
			Assert::IsFalse(_library->find(finished[1]).has_value());
			Assert::IsTrue(_library->find(finished[2]).has_value());
			Assert::IsTrue(_library->find(finished[4]).has_value());
			Assert::IsTrue(_library->find(unfinished).has_value(), L"the unfinished one, older than all, stays");

			_library->setKeepCount(1); // a smaller cap trims at once
			Assert::AreEqual(2, _library->count());
			Assert::IsTrue(_library->find(finished[4]).has_value());
			Assert::IsTrue(_library->find(unfinished).has_value());
		}

		TEST_METHOD(EachProfileHasItsOwnGames)
		{
			const QString alice = addUser("Alice");
			_library->put(makeRecord("local", false, { "e2e4" }));
			const QString bob = addUser("Bob"); // createUser logs in: the library follows
			Assert::AreEqual(0, _library->count(), L"Bob has none");
			_library->put(makeRecord("computer", false, { "d2d4" }));
			_library->put(makeRecord("computer", true, { "c2c4" }));
			Assert::AreEqual(2, _library->count());
			_users->login(alice);
			Assert::AreEqual(1, _library->count());
			Assert::AreEqual(QStringLiteral("e2e4"), _library->records()[0].moves.join(' '));
			_users->logout();
			Assert::AreEqual(0, _library->count(), L"nobody logged in: nothing");
			Assert::AreEqual(2, jsonFilesIn(GameLibrary::gamesDirOf(bob)));
			_users->login(bob);
			_users->deleteUser(bob);
			Assert::IsFalse(QDir(GameLibrary::gamesDirOf(bob)).exists(), L"the profile's folder went with it");
			Assert::AreEqual(0, _library->count());
		}

		// A games.json of BL-19 that can't be read as JSON is kept aside (nothing migrated from it).
		TEST_METHOD(ADamagedLegacyFileIsKeptAside)
		{
			const QString user = addUser("Alice");
			writeFile(GameLibrary::legacyFilePathOf(user), "{ not json");
			makeLibrary();
			Assert::AreEqual(0, _library->count());
			Assert::IsTrue(QFile::exists(GameLibrary::legacyFilePathOf(user) + ".damaged"), L"kept aside");
			Assert::IsFalse(QFile::exists(GameLibrary::legacyFilePathOf(user)));
			_library->put(makeRecord("local", false, { "d2d4" }));
			makeLibrary();
			Assert::AreEqual(1, _library->count(), L"games are kept as before");
		}

		// BL-20: games.json (BL-19) becomes a signed file per game once, the rating rating.json; it is then
		// kept as games.json.migrated and never read again.
		TEST_METHOD(TheLegacyFileIsMigratedOnceAndSigned)
		{
			const QString user = addUser("Alice");
			GameRecord a = makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-2));
			a.id = QStringLiteral("game-a");
			a.rated = true; a.ratingBefore = 1200; a.ratingChange = 27;
			GameRecord b2 = makeRecord("local", false, { "d2d4" }, QDateTime::currentDateTime().addDays(-1));
			b2.id = QStringLiteral("game-b");
			writeFile(GameLibrary::legacyFilePathOf(user), legacyContent({ a, b2 }, 1227, 1));
			makeLibrary();
			Assert::AreEqual(2, _library->count());
			Assert::AreEqual(1227, _library->rating());
			Assert::AreEqual(1, _library->ratedGames());
			Assert::AreEqual(2, jsonFilesIn(GameLibrary::gamesDirOf(user)), L"a file per game");
			Assert::IsTrue(QFile::exists(GameLibrary::ratingFilePathOf(user)));
			Assert::IsFalse(QFile::exists(GameLibrary::legacyFilePathOf(user)));
			Assert::IsTrue(QFile::exists(GameLibrary::legacyFilePathOf(user) + ".migrated"), L"kept, not read again");
			Assert::IsTrue(GameLibrary::readGameFile(GameLibrary::gameFilePathOf(user, "game-a")).has_value(), L"signed");
			Assert::AreEqual(0, _library->setAsideCount());

			makeLibrary(); // no second migration
			Assert::AreEqual(2, _library->count());
			Assert::AreEqual(1227, _library->rating());
		}

		// BL-20: a game file edited by hand (here: the result) fails its check: set aside in games/rejected/,
		// not loaded, said once (gamesSetAside); the other games load as before.
		TEST_METHOD(AnEditedGameIsSetAside)
		{
			const QString user = addUser("Alice");
			const QString kept = _library->put(makeRecord("local", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-1)));
			const QString edited = _library->put(makeRecord("computer", true, { "d2d4" }));
			_library->flush();
			const QString path = GameLibrary::gameFilePathOf(user, edited);
			QByteArray content = readFile(path);
			Assert::IsTrue(content.contains("\"result\":\"1-0\""));
			content.replace("\"result\":\"1-0\"", "\"result\":\"0-1\"");
			writeFile(path, content);

			_users = std::make_unique<UserManager>();
			_library = std::make_unique<GameLibrary>();
			QSignalSpy setAside(_library.get(), &GameLibrary::gamesSetAside);
			_library->setUserManager(_users.get());
			Assert::AreEqual(1, _library->count(), L"only the one that verifies");
			Assert::IsTrue(_library->find(kept).has_value());
			Assert::IsFalse(_library->find(edited).has_value());
			Assert::AreEqual(1, _library->setAsideCount());
			Assert::AreEqual(1, (int)setAside.count());
			Assert::AreEqual(1, setAside.at(0).at(0).toInt());
			Assert::IsFalse(QFile::exists(path), L"moved, not deleted");
			Assert::AreEqual(1, jsonFilesIn(GameLibrary::rejectedDirOf(user)));

			makeLibrary();
			Assert::AreEqual(0, _library->setAsideCount(), L"said once: it is in rejected/ now");
			Assert::AreEqual(1, _library->count());
		}

		// A good file copied under another game's name, a file that isn't JSON, one without a signature, and
		// one signed with a key this copy doesn't know: all set aside.
		TEST_METHOD(OtherFailingFilesAreSetAside)
		{
			const QString user = addUser("Alice");
			const QString id = _library->put(makeRecord("local", true, { "e2e4" }));
			_library->flush();
			const QByteArray good = readFile(GameLibrary::gameFilePathOf(user, id));
			writeFile(GameLibrary::gameFilePathOf(user, "copied"), good);
			writeFile(GameLibrary::gameFilePathOf(user, "garbage"), "not json");
			QJsonObject unsigned_ = QJsonDocument::fromJson(good).object();
			unsigned_.remove("sig");
			writeFile(GameLibrary::gameFilePathOf(user, "unsigned"), QJsonDocument(unsigned_).toJson());
			QJsonObject newer = QJsonDocument::fromJson(good).object();
			newer["sig"] = QStringLiteral("9:") + newer.value("sig").toString().mid(2);
			writeFile(GameLibrary::gameFilePathOf(user, "newerkey"), QJsonDocument(newer).toJson());
			GameLibrary::ReadFailure why = GameLibrary::ReadFailure::None;
			Assert::IsFalse(GameLibrary::readGameFile(GameLibrary::gameFilePathOf(user, "copied"), &why).has_value());
			Assert::IsTrue(why == GameLibrary::ReadFailure::OtherId);
			Assert::IsFalse(GameLibrary::readGameFile(GameLibrary::gameFilePathOf(user, "garbage"), &why).has_value());
			Assert::IsTrue(why == GameLibrary::ReadFailure::NotJson);
			Assert::IsFalse(GameLibrary::readGameFile(GameLibrary::gameFilePathOf(user, "unsigned"), &why).has_value());
			Assert::IsTrue(why == GameLibrary::ReadFailure::BadSignature);
			Assert::IsFalse(GameLibrary::readGameFile(GameLibrary::gameFilePathOf(user, "missing"), &why).has_value());
			Assert::IsTrue(why == GameLibrary::ReadFailure::CantOpen, L"a lock: left where it is, not set aside");

			makeLibrary();
			Assert::AreEqual(1, _library->count());
			Assert::AreEqual(4, _library->setAsideCount());
			Assert::AreEqual(4, jsonFilesIn(GameLibrary::rejectedDirOf(user)));
		}

		// BL-20: a move writes only its own game's file; Delete removes the file.
		TEST_METHOD(OnlyTheChangedGameIsWritten)
		{
			const QString user = addUser("Alice");
			const QString a = _library->put(makeRecord("local", false, { "e2e4" }, QDateTime::currentDateTime().addDays(-1)));
			const QString b3 = _library->put(makeRecord("local", false, { "d2d4" }));
			_library->flush();
			const QDateTime aWritten = QFileInfo(GameLibrary::gameFilePathOf(user, a)).lastModified();
			const QByteArray aBefore = readFile(GameLibrary::gameFilePathOf(user, a));
			GameRecord next = *_library->find(b3);
			next.moves.append(QStringLiteral("d7d5"));
			_library->put(next);
			_library->flush();
			Assert::IsTrue(aBefore == readFile(GameLibrary::gameFilePathOf(user, a)), L"the other game untouched");
			Assert::IsTrue(aWritten == QFileInfo(GameLibrary::gameFilePathOf(user, a)).lastModified());
			Assert::IsTrue(readFile(GameLibrary::gameFilePathOf(user, b3)).contains("d7d5"));
			Assert::IsTrue(_library->remove(a));
			_library->flush();
			Assert::IsFalse(QFile::exists(GameLibrary::gameFilePathOf(user, a)), L"Delete removes its file");
		}

		// The two save slots of versions before BL-19 (chess.autosave: the robot's game, chess.save: Save game,
		// both the binary format with their info in users.json) become records on the first load; the files go.
		TEST_METHOD(TheOldSaveSlotsAreMigratedOnce)
		{
			const QString user = addUser("Alice");
			// The robot's autosave: 1. e4 e5, the player White, level 4, a clock.
			{
				Chess::Game game;
				game.DoMove(Chess::Move::Parse("e2e4"));
				game.DoMove(Chess::Move::Parse("e7e5"));
				Assert::IsTrue(game.Save(AppConfig::autoSaveFilePath(user).toStdString()));
				UserManager::GameSaveInfo info;
				info.isSinglePlayer = true;
				info.playerPlaysWhite = true;
				info.opponentName = "Robot";
				info.computerLevel = 4;
				info.startDate = QDate(2026, 10, 30);
				info.clockLimitSec = 600; info.clockIncSec = 5; info.whiteMs = 500000; info.blackMs = 480000;
				_users->setAutoSaveInfo(info);
			}
			// The saved game: Fool's mate between two players, Bob (a profile that no longer exists) as Black.
			{
				Chess::Game game;
				for (const char* m : { "f2f3", "e7e5", "g2g4", "d8h4" })
					game.DoMove(Chess::Move::Parse(m));
				Assert::IsTrue(game.Save(AppConfig::savedGameFilePath(user).toStdString()));
				UserManager::GameSaveInfo info;
				info.isSinglePlayer = false;
				info.playerPlaysWhite = true;
				info.opponentName = "Bob";
				info.opponentUserId = "gone";
				_users->setSavedGameInfo(info);
			}
			makeLibrary();
			Assert::AreEqual(2, _library->count());
			Assert::IsFalse(QFile::exists(AppConfig::autoSaveFilePath(user)), L"the autosave file went");
			Assert::IsFalse(QFile::exists(AppConfig::savedGameFilePath(user)), L"the save file went");
			Assert::AreEqual(2, jsonFilesIn(GameLibrary::gamesDirOf(user)), L"a signed file per game");
			Assert::IsFalse(_users->autoSaveInfo().isSinglePlayer, L"the old info cleared");
			Assert::IsTrue(_users->savedGameInfo().opponentName.isEmpty());

			const auto unfinished = _library->unfinished();
			Assert::AreEqual(1, (int)unfinished.size());
			const GameRecord& robot = unfinished[0];
			Assert::AreEqual(QStringLiteral("computer"), robot.kind);
			Assert::AreEqual(QStringLiteral("e2e4 e7e5"), robot.moves.join(' '));
			Assert::AreEqual(4, robot.level);
			Assert::AreEqual(QStringLiteral("Robot"), robot.opponentName);
			Assert::AreEqual(QStringLiteral("2026-10-30"), robot.startedAt.date().toString(Qt::ISODate));
			Assert::AreEqual(600, robot.clockLimitSec);
			Assert::AreEqual((qint64)480000, robot.blackMs);
			Assert::IsTrue(robot.playerToMove());
			Assert::IsTrue(robot.fen.startsWith(QStringLiteral("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w")));

			GameRecord mate;
			for (const auto& r : _library->records())
				if (r.kind == QLatin1String("local"))
					mate = r;
			Assert::IsTrue(mate.finished, L"the saved game ended in mate: finished");
			Assert::AreEqual(QStringLiteral("0-1"), mate.result);
			Assert::AreEqual(QStringLiteral("Bob"), mate.opponentName);
			Assert::AreEqual(QStringLiteral("gone"), mate.opponentUserId);
			Assert::AreEqual(4, mate.plyCount());

			makeLibrary(); // no second migration, nothing doubled
			Assert::AreEqual(2, _library->count());
		}

		TEST_METHOD(AnUnfinishedOldSaveCanGoOn)
		{
			const QString user = addUser("Alice");
			Chess::Game game;
			game.DoMove(Chess::Move::Parse("d2d4"));
			Assert::IsTrue(game.Save(AppConfig::savedGameFilePath(user).toStdString()));
			UserManager::GameSaveInfo info;
			info.isSinglePlayer = false;
			info.opponentName = "Guest";
			_users->setSavedGameInfo(info);
			makeLibrary();
			Assert::AreEqual(1, _library->count());
			Assert::IsFalse(_library->records()[0].finished, L"not over: Continue offers it");
			Assert::AreEqual(QStringLiteral("local"), _library->records()[0].kind);
		}

		// BL-20: {"record": …, "sig": "1:<hex>"}, compact; the record as GameRecord::toJson writes it.
		TEST_METHOD(TheGameFileIsSignedCompactJson)
		{
			const QString user = addUser("Alice");
			const QString id = _library->put(makeRecord("local", false, { "e2e4" }));
			_library->flush();
			const QByteArray content = readFile(GameLibrary::gameFilePathOf(user, id));
			Assert::IsFalse(content.contains('\n'), L"compact");
			const QJsonObject root = QJsonDocument::fromJson(content).object();
			Assert::IsTrue(root.value("sig").toString().startsWith("1:"), L"key 1");
			const QJsonObject g = root.value("record").toObject();
			Assert::AreEqual(id, g.value("id").toString());
			Assert::AreEqual(QStringLiteral("e2e4"), g.value("moves").toArray()[0].toString());
			Assert::IsFalse(g.contains("result"), L"no result while unfinished");
			Assert::IsFalse(g.contains("analysis"), L"nothing until analysed");
		}
		// A games.json (BL-19) that is there but can't be read now (here: a folder of that name) is never
		// written over; games kept meanwhile get their files, and once it can be read it is migrated, its
		// games beside them.
		TEST_METHOD(ALegacyFileThatCantBeReadIsMigratedLater)
		{
			const QString user = addUser("Alice");
			const QString legacy = GameLibrary::legacyFilePathOf(user);
			writeFile(legacy + ".held", legacyContent({ makeRecord("local", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-2)) }));
			Assert::IsTrue(QDir().mkdir(legacy), L"a folder in its place: it can't be opened");
			makeLibrary();
			Assert::AreEqual(0, _library->count(), L"its games can't be shown");
			_library->put(makeRecord("local", false, { "d2d4" }));
			_library->flush();
			Assert::IsTrue(QFileInfo(legacy).isDir(), L"not written over");

			// It can be read again: the next write migrates it.
			Assert::IsTrue(QDir().rmdir(legacy));
			Assert::IsTrue(QFile::rename(legacy + ".held", legacy));
			_library->put(makeRecord("local", false, { "c2c4" }));
			_library->flush();
			Assert::AreEqual(3, _library->count(), L"the file's game and the two kept meanwhile");
			Assert::IsTrue(QFile::exists(legacy + ".migrated"));
			makeLibrary();
			Assert::AreEqual(3, _library->count());
		}

		// An old save that can't be read is kept aside, not deleted (review #5); the autosave of a
		// Lichess game isn't migrated: those versions never offered it (review #1).
		TEST_METHOD(OldSavesThatCantBeMigrated)
		{
			const QString user = addUser("Alice");
			{
				QFile f(AppConfig::savedGameFilePath(user));
				Assert::IsTrue(f.open(QIODevice::WriteOnly));
				f.write("not a saved game");
			}
			{
				Chess::Game game;
				game.DoMove(Chess::Move::Parse("e2e4"));
				Assert::IsTrue(game.Save(AppConfig::autoSaveFilePath(user).toStdString()));
				UserManager::GameSaveInfo info;
				info.isSinglePlayer = false; // a Lichess game's
				info.opponentName = "Lichess opponent";
				_users->setAutoSaveInfo(info);
			}
			makeLibrary();
			Assert::AreEqual(0, _library->count(), L"neither became a game");
			Assert::IsTrue(QFile::exists(AppConfig::savedGameFilePath(user) + ".damaged"), L"the unreadable save kept aside");
			Assert::IsFalse(QFile::exists(AppConfig::savedGameFilePath(user)));
			Assert::IsFalse(QFile::exists(AppConfig::autoSaveFilePath(user)), L"the Lichess autosave gone");
		}

		// "Games kept" is every profile's: finishedBeyond counts them all, and a smaller keepCount trims the
		// other profiles' files at once (review #2).
		TEST_METHOD(KeepCountCountsAndTrimsEveryProfile)
		{
			const QString alice = addUser("Alice");
			for (int i = 0; i < 3; ++i)
				_library->put(makeRecord("local", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-10 + i)));
			const QString bob = addUser("Bob"); // the library follows: Bob's now (Alice's written)
			for (int i = 0; i < 5; ++i)
				_library->put(makeRecord("local", true, { "d2d4" }, QDateTime::currentDateTime().addDays(-10 + i)));
			_library->put(makeRecord("local", false, { "c2c4" }));
			Assert::AreEqual(0, _library->finishedBeyond(5));
			Assert::AreEqual(1 + 3, _library->finishedBeyond(2), L"Alice 1, Bob 3");
			_library->setKeepCount(2);
			Assert::AreEqual(3, _library->count(), L"Bob: 2 finished, the unfinished one");
			_users->login(alice);
			Assert::AreEqual(2, _library->count(), L"Alice's file trimmed too");
			Assert::AreEqual(0, _library->finishedBeyond(2));
		}

		// A move from an empty square isn't played (the game itself only asserts on it): false, no crash.
		TEST_METHOD(PlayStopsAtAMoveThatCantBePlayed)
		{
			Assert::IsTrue(GameRecord::play({ "e2e4", "e7e5" }) != nullptr);
			Assert::IsTrue(GameRecord::play({ "e2e4", "e2e4" }) == nullptr, L"from an empty square");
			Assert::IsTrue(GameRecord::play({ "e2e4", "e7e5", "e4e6" }) == nullptr, L"an illegal move");
			Chess::Game game;
			game.Restart();
			Assert::IsFalse(GameRecord::playOn(game, { "e2e4", "a3a4", "e7e5" }));
			Assert::AreEqual(1, game.GetMoveCount(), L"the moves before it stay");
		}

		// LI-01A: a Chess960 game keeps its start position (variant, startFen in games.json) and is replayed
		// from it: #3 BQNNRKRB, O-O as the king onto its rook (f1g1). A standard record has neither field.
		TEST_METHOD(Chess960_KeptWithItsStartPosition)
		{
			addUser("Alice");
			GameRecord r = makeRecord("local", false, {});
			r.setStartPosition(Chess::StartPosition::Chess960(3));
			Assert::IsTrue(r.chess960());
			Assert::IsTrue(r.setMoves({ "f1g1", "f8g8", "e2e4" }), L"played from #3");
			Assert::IsTrue(r.fen.startsWith(QStringLiteral("bqnnrrkb/pppppppp/8/8/4P3/8/PPPP1PPP/BQNNRRKB b")), r.fen.toStdWString().c_str());
			const QString id = _library->put(r);

			const QJsonObject json = _library->find(id)->toJson();
			Assert::AreEqual(QStringLiteral("chess960"), json.value("variant").toString());
			Assert::AreEqual(QString::fromStdString(Chess::StartPosition::Chess960(3).Fen()), json.value("startFen").toString());
			const GameRecord read = GameRecord::fromJson(json);
			Assert::IsTrue(read.chess960());
			Assert::AreEqual(3, read.startPosition().Number());
			const auto game = read.replay();
			Assert::IsTrue(game != nullptr, L"its moves play from its start");
			Assert::AreEqual(3, game->GetMoveCount());
			Assert::IsTrue(read.replay(1)->GetPieceAt(Chess::g1).Type == Chess::KING, L"the first move: O-O");
			const QVariantMap card = read.toVariant();
			Assert::IsTrue(card.value("chess960").toBool());
			Assert::AreEqual(3, card.value("startPosition").toInt());

			Assert::IsTrue(GameRecord::play({ "f1g1" }) == nullptr, L"not a move of the standard game");
			const GameRecord standard = makeRecord("local", false, { "e2e4" });
			Assert::IsFalse(standard.toJson().contains("variant"));
			Assert::IsFalse(standard.toJson().contains("startFen"));
			Assert::AreEqual(-1, standard.toVariant().value("startPosition").toInt());
		}

		// A Chess960 record whose start can't be read (a damaged or hand-edited games.json) stays a Chess960
		// record: never replayed from the standard start (its moves are Chess960 moves), so continue and
		// open refuse it, and it is written back as it was read, not downgraded to a standard game.
		TEST_METHOD(Chess960_UnreadableStart_KeptNotDowngraded)
		{
			QJsonObject json = makeRecord("local", false, {}).toJson();
			json["variant"] = QStringLiteral("chess960");
			json["startFen"] = QStringLiteral("bqnnrkrb/pppppppp/8/8");   // cut short
			json["moves"] = QJsonArray{ "f1g1", "f8g8" };
			const GameRecord read = GameRecord::fromJson(json);
			Assert::IsTrue(read.chess960());
			Assert::IsTrue(read.replay() == nullptr, L"not played from the standard start");
			GameRecord copy = read;
			Assert::IsFalse(copy.setMoves({ "e2e4" }), L"no moves without its start");
			const QJsonObject again = read.toJson();
			Assert::AreEqual(QStringLiteral("chess960"), again.value("variant").toString());
			Assert::AreEqual(QStringLiteral("bqnnrkrb/pppppppp/8/8"), again.value("startFen").toString(), L"as it was read");
			Assert::AreEqual(-1, read.toVariant().value("startPosition").toInt());

			json.remove("startFen"); // the variant without its start
			const GameRecord noStart = GameRecord::fromJson(json);
			Assert::IsTrue(noStart.chess960());
			Assert::IsTrue(noStart.replay() == nullptr);
			Assert::AreEqual(QStringLiteral("chess960"), noStart.toJson().value("variant").toString());
		}

		// ── The rating vs the robot (step 5) ──

		// Elo from 1200 against level 3 (1320): E = 0.334; K 40 under 30 games, 20 from then on.
		TEST_METHOD(RatingChangesAreElo)
		{
			const LocalRating start;
			Assert::AreEqual(1200, start.value());
			Assert::IsTrue(start.provisional());
			Assert::AreEqual(27, start.changeFor(1320, 1.0), L"a win");
			Assert::AreEqual(7, start.changeFor(1320, 0.5), L"a draw");
			Assert::AreEqual(-13, start.changeFor(1320, 0.0), L"a loss");
			Assert::AreEqual(0, LocalRating(1200, 3).changeFor(1200, 0.5), L"a draw between equals");
			Assert::AreEqual(40, LocalRating(1200, 29).kFactor());
			Assert::AreEqual(20, LocalRating(1200, 30).kFactor());
			Assert::IsTrue(LocalRating(1200, 9).provisional());
			Assert::IsFalse(LocalRating(1200, 10).provisional());
			LocalRating r;
			r.apply(27);
			r.revert(27);
			Assert::IsTrue(r == LocalRating(), L"a change given back restores it");
		}

		// A finished game vs the robot is rated when it is kept, once; its change is on its record and the
		// rating in the file.
		TEST_METHOD(AFinishedRobotGameIsRatedOnce)
		{
			const QString user = addUser("Alice");
			QSignalSpy changed(_library.get(), &GameLibrary::ratingChanged);
			const QString id = _library->put(makeRecord("computer", true, { "e2e4", "e7e5" })); // White (the player) won
			Assert::AreEqual(1227, _library->rating());
			Assert::AreEqual(1, _library->ratedGames());
			Assert::AreEqual(1, (int)changed.count());
			auto kept = _library->find(id);
			Assert::IsTrue(kept->rated);
			Assert::AreEqual(1200, kept->ratingBefore);
			Assert::AreEqual(27, kept->ratingChange);

			_library->put(*kept); // written again (left, the PGN, ...): not again
			Assert::AreEqual(1227, _library->rating());
			Assert::AreEqual(1, _library->ratedGames());

			GameRecord lost = makeRecord("computer", true, { "d2d4", "d7d5" });
			lost.result = QStringLiteral("1-0");
			lost.playerPlaysWhite = false;
			_library->put(lost);
			Assert::AreEqual(1227 - 15, _library->rating(), L"a loss from 1227 against 1320");
			_library->flush();

			makeLibrary();
			Assert::AreEqual(1212, _library->rating(), L"read from the file");
			Assert::AreEqual(2, _library->ratedGames());
			kept = _library->find(id);
			Assert::IsTrue(kept->rated);
			Assert::AreEqual(27, kept->ratingChange);
			Assert::AreEqual(27, kept->toVariant().value("ratingChange").toInt());
		}

		// Only games vs the robot without a takeback, not begun from another game, with a result.
		TEST_METHOD(GamesThatArentRated)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("local", true, { "e2e4", "e7e5" }));
			_library->put(makeRecord("computer", false, { "e2e4", "e7e5" }));
			GameRecord undone = makeRecord("computer", true, { "e2e4", "e7e5" });
			undone.undoCount = 1;
			_library->put(undone);
			GameRecord fork = makeRecord("computer", true, { "e2e4", "e7e5" });
			fork.forkOfId = QStringLiteral("other");
			fork.forkPly = 1;
			_library->put(fork);
			// LI-01A: a Chess960 game isn't rated (the owner's choice).
			GameRecord chess960 = makeRecord("computer", true, { "f1g1", "f8g8" });
			chess960.setStartPosition(Chess::StartPosition::Chess960(3));
			Assert::IsFalse(chess960.rateable());
			_library->put(chess960);
			// As lichess.org: a game ended before both sides moved is aborted, not rated (review of #167-#170).
			GameRecord aborted = makeRecord("computer", true, { "e2e4" });
			aborted.result = QStringLiteral("0-1"); // the player resigned after 1. e4
			_library->put(aborted);
			Assert::AreEqual(1200, _library->rating());
			Assert::AreEqual(0, _library->ratedGames());
			for (const auto& r : _library->records())
				Assert::IsFalse(r.rated);
			_library->flush();
			Assert::IsFalse(QFile::exists(GameLibrary::ratingFilePathOf(user)), L"no rating file until a game is rated");
		}

		// A takeback after the result resumes the game: its change goes back, and with a takeback it is
		// never rated again.
		TEST_METHOD(ATakebackGivesTheChangeBack)
		{
			addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-1)));
			const QString id = _library->put(makeRecord("computer", true, { "e2e4", "e7e5", "g1f3" }));
			Assert::AreEqual(1200 + 27 + 25, _library->rating());
			GameRecord resumed = *_library->find(id);
			resumed.finished = false;
			resumed.result.clear();
			resumed.undoCount = 1;
			_library->put(resumed);
			Assert::AreEqual(1227, _library->rating());
			Assert::AreEqual(1, _library->ratedGames());
			Assert::IsFalse(_library->find(id)->rated);

			resumed = *_library->find(id);
			resumed.finished = true;
			resumed.result = QStringLiteral("1-0");
			_library->put(resumed);
			Assert::AreEqual(1227, _library->rating(), L"finished again after a takeback: not rated");
		}

		// While games.json (BL-19) can't be read the rating isn't known (it shows the start rating): a robot
		// game that ends then is rated once the file is read and migrated, from the real rating.
		TEST_METHOD(AGameEndedWhileTheLegacyFileCantBeReadIsRatedLater)
		{
			const QString user = addUser("Alice");
			const QString legacy = GameLibrary::legacyFilePathOf(user);
			GameRecord old = makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-1));
			old.rated = true; old.ratingBefore = 1200; old.ratingChange = 27;
			writeFile(legacy + ".held", legacyContent({ old }, 1227, 1));
			Assert::IsTrue(QDir().mkdir(legacy));
			makeLibrary();
			Assert::AreEqual(0, _library->ratedGames(), L"not known now");

			const QString id = _library->put(makeRecord("computer", true, { "d2d4", "d7d5" }));
			Assert::IsFalse(_library->find(id)->rated, L"not from the start rating");
			Assert::AreEqual(0, _library->ratedGames());
			_library->flush();
			Assert::IsFalse(QFile::exists(GameLibrary::ratingFilePathOf(user)), L"no rating written while it isn't known");

			Assert::IsTrue(QDir().rmdir(legacy));
			Assert::IsTrue(QFile::rename(legacy + ".held", legacy));
			_library->flush(); // read, migrated, rated, written
			const auto rated = _library->find(id);
			Assert::IsTrue(rated->rated, L"rated now");
			Assert::AreEqual(1227, rated->ratingBefore, L"from the real rating");
			Assert::AreEqual(1227 + 25, _library->rating());
			Assert::AreEqual(2, _library->ratedGames());
			makeLibrary();
			Assert::AreEqual(1227 + 25, _library->rating(), L"in rating.json");
			Assert::IsTrue(_library->find(id)->rated, L"and on its record");
		}

		// Removing games, by hand or by the cap, never changes the rating; another profile's file keeps
		// its own when the cap trims it.
		TEST_METHOD(RemovingGamesKeepsTheRating)
		{
			const QString alice = addUser("Alice");
			for (int i = 0; i < 3; ++i)
				_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-10 + i)));
			const int aliceRating = _library->rating();
			Assert::AreEqual(3, _library->ratedGames());
			const QString bob = addUser("Bob");
			Assert::AreEqual(1200, _library->rating(), L"Bob's own");
			const QString lost = _library->put(makeRecord("computer", true, { "d2d4", "d7d5" }));
			const int bobRating = _library->rating();
			Assert::IsTrue(_library->remove(lost));
			Assert::AreEqual(bobRating, _library->rating(), L"Delete");
			_library->setKeepCount(1); // Alice's file is trimmed too
			Assert::AreEqual(bobRating, _library->rating());
			_users->login(alice);
			Assert::AreEqual(1, _library->count());
			Assert::AreEqual(aliceRating, _library->rating(), L"the cap");
			Assert::AreEqual(3, _library->ratedGames());
		}

		// ── The rating signed (BL-20 step 2) ──

		// rating.json is signed; each rated record has the rated games after it, so the newest one gives
		// the whole rating. A new profile has nothing to restore.
		TEST_METHOD(TheRatingFileIsSignedAndEachRatedGameCountsItsRatedGames)
		{
			const QString user = addUser("Alice");
			Assert::IsFalse(_library->ratingRestored(), L"a new profile");
			const QString first = _library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-1)));
			const QString second = _library->put(makeRecord("computer", true, { "d2d4", "d7d5" }));
			_library->flush();
			Assert::AreEqual(1, _library->find(first)->ratedGames);
			Assert::AreEqual(2, _library->find(second)->ratedGames);
			const auto read = GameLibrary::readRatingFile(GameLibrary::ratingFilePathOf(user));
			Assert::IsTrue(read.has_value(), L"it verifies");
			Assert::IsTrue(*read == _library->localRating());
			Assert::IsTrue(readFile(GameLibrary::ratingFilePathOf(user)).contains("\"sig\":\"1:"));

			const auto rating = GameLibrary::ratingFromGames(_library->records());
			Assert::IsTrue(rating.has_value());
			Assert::IsTrue(*rating == _library->localRating(), L"the newest rated game gives the whole rating");

			makeLibrary();
			Assert::AreEqual(2, _library->find(second)->ratedGames, L"read back");
			Assert::IsFalse(_library->ratingRestored());
		}

		// An edited rating.json (here: the value, the signature kept) fails its check: set aside in
		// games/rejected/, the rating restored from the newest rated game, written signed again, said once.
		TEST_METHOD(AnEditedRatingIsRestoredFromTheNewestRatedGame)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-1)));
			_library->put(makeRecord("computer", true, { "d2d4", "d7d5" }));
			const LocalRating real = _library->localRating();
			_library->flush();
			const QString path = GameLibrary::ratingFilePathOf(user);
			QByteArray content = readFile(path);
			const QByteArray value = "\"value\":" + QByteArray::number(real.value());
			Assert::IsTrue(content.contains(value));
			content.replace(value, "\"value\":2400");
			writeFile(path, content);

			makeLibrary();
			Assert::IsTrue(_library->ratingRestored());
			Assert::IsTrue(_library->localRating() == real, L"from the newest rated game");
			Assert::AreEqual(1, jsonFilesIn(GameLibrary::rejectedDirOf(user)), L"the edited file kept aside");
			Assert::AreEqual(2, _library->count(), L"the games stay");
			_library->flush();
			Assert::IsTrue(GameLibrary::readRatingFile(path).has_value(), L"written signed again");
			makeLibrary();
			Assert::IsFalse(_library->ratingRestored(), L"once");
			Assert::IsTrue(_library->localRating() == real);
		}

		// One without a signature, or not JSON, fails too; with no rated game kept the rating starts again.
		// A rating.json deleted while rated games are kept is restored from them.
		TEST_METHOD(AFailingRatingWithoutRatedGamesStartsAgain_AMissingOneIsRestored)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("local", true, { "e2e4", "e7e5" }));
			_library->flush();
			writeFile(GameLibrary::ratingFilePathOf(user), R"({"version":1,"rating":{"value":2400,"games":50}})");
			makeLibrary();
			Assert::IsTrue(_library->ratingRestored());
			Assert::AreEqual(LocalRating::Start, _library->rating());
			Assert::AreEqual(0, _library->ratedGames());

			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }));
			const LocalRating real = _library->localRating();
			_library->flush();
			Assert::IsTrue(QFile::remove(GameLibrary::ratingFilePathOf(user)));
			makeLibrary();
			Assert::IsTrue(_library->ratingRestored(), L"rated games kept, no rating file");
			Assert::IsTrue(_library->localRating() == real);
			_library->flush(); // the restored rating written now, not over the file below when this library goes

			writeFile(GameLibrary::ratingFilePathOf(user), "not json");
			makeLibrary();
			Assert::IsTrue(_library->ratingRestored());
			Assert::IsTrue(_library->localRating() == real);
		}

		// The rating is written after its game (two writes): a rating.json that verifies but is behind the
		// newest rated game (a crash between them) catches up silently.
		TEST_METHOD(ARatingBehindTheNewestRatedGameCatchesUp)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-2)));
			const LocalRating afterFirst = _library->localRating();
			_library->put(makeRecord("computer", true, { "d2d4", "d7d5" }, QDateTime::currentDateTime().addDays(-1)));
			_library->put(makeRecord("computer", true, { "c2c4", "c7c5" }));
			const LocalRating real = _library->localRating();
			_library->flush();
			writeFile(GameLibrary::ratingFilePathOf(user), GameLibrary::ratingFileContent(afterFirst));
			makeLibrary();
			Assert::IsFalse(_library->ratingRestored(), L"not a failed check");
			Assert::IsTrue(_library->localRating() == real, L"two games behind (rating.json couldn't be written): both");
		}

		// The catch-up follows only the games that go on from the rating (review of #187-#190): a rated
		// game of another profile copied here verifies, but doesn't continue this rating.
		TEST_METHOD(ACopiedRatedGameDoesntMoveTheRating)
		{
			const QString alice = addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-5)));
			const LocalRating aliceRating = _library->localRating();
			const QString bob = addUser("Bob");
			QString newest;
			for (int i = 0; i < 3; ++i)
				newest = _library->put(makeRecord("computer", true, { "e2e4", "e7e5" }, QDateTime::currentDateTime().addDays(-3 + i)));
			Assert::AreEqual(3, _library->find(newest)->ratedGames);
			_library->flush();
			Assert::IsTrue(QFile::copy(GameLibrary::gameFilePathOf(bob, newest), GameLibrary::gameFilePathOf(alice, newest)));

			_users->login(alice);
			Assert::AreEqual(2, _library->count(), L"the copy verifies: it is a game");
			Assert::IsTrue(_library->localRating() == aliceRating, L"but not Alice's rating");
			Assert::IsFalse(_library->ratingRestored());
		}

		// ── games.json beside signed files, a remove retried, a migration retried (review of #187-#190) ──

		// games.json (BL-19) is trusted once, as this PC's own: beside files of BL-20 it was written by hand
		// (a way to sign any rating). Set aside, said as a file that failed its check; nothing in it loaded.
		TEST_METHOD(ALegacyFileBesideSignedFilesIsSetAside)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }));
			const LocalRating real = _library->localRating();
			_library->flush();
			GameRecord fake = makeRecord("computer", true, { "d2d4", "d7d5" });
			fake.rated = true; fake.ratingBefore = 2480; fake.ratingChange = 20;
			writeFile(GameLibrary::legacyFilePathOf(user), legacyContent({ fake }, 2500, 60));

			_users = std::make_unique<UserManager>();
			_library = std::make_unique<GameLibrary>();
			QSignalSpy setAside(_library.get(), &GameLibrary::gamesSetAside);
			_library->setUserManager(_users.get());
			Assert::IsTrue(_library->localRating() == real, L"not its rating");
			Assert::AreEqual(1, _library->count(), L"not its games");
			Assert::AreEqual(1, _library->setAsideCount());
			Assert::AreEqual(1, (int)setAside.count());
			Assert::IsFalse(QFile::exists(GameLibrary::legacyFilePathOf(user)));
			Assert::IsTrue(QFile::exists(GameLibrary::rejectedDirOf(user) + "/games.json"), L"kept for a look");
			Assert::IsFalse(QFile::exists(GameLibrary::legacyFilePathOf(user) + ".migrated"));

			// A game file alone (no rating yet) is enough: the profile is BL-20's.
			const QString bob = addUser("Bob");
			_library->put(makeRecord("local", false, { "e2e4" }));
			_library->flush();
			writeFile(GameLibrary::legacyFilePathOf(bob), legacyContent({ fake }, 2500, 60));
			_users->login(user);
			_users->login(bob);
			Assert::AreEqual(LocalRating::Start, _library->rating());
			Assert::AreEqual(1, _library->count());
			Assert::AreEqual(1, _library->setAsideCount());
		}

		// A failing rating.json beside a games.json that can't be read now: the rating is restored and said
		// at once (review of #187-#190: it waited for games.json and went back to 1200 without a word).
		TEST_METHOD(AFailingRatingBesideALockedLegacyFileIsRestoredAtOnce)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4", "e7e5" }));
			const LocalRating real = _library->localRating();
			_library->flush();
			writeFile(GameLibrary::ratingFilePathOf(user), "not json");
			Assert::IsTrue(QDir().mkdir(GameLibrary::legacyFilePathOf(user)), L"a folder in its place: it can't be opened");
			makeLibrary();
			Assert::IsTrue(_library->ratingRestored());
			Assert::IsTrue(_library->localRating() == real);
		}

		// A game file that can't be removed now (another program holds it) is removed at the next write;
		// else the deleted game would come back at the next start.
		TEST_METHOD(ARemoveThatFailsIsTriedAgain)
		{
			const QString user = addUser("Alice");
			const QString id = _library->put(makeRecord("local", true, { "e2e4" }));
			_library->flush();
			const QString path = GameLibrary::gameFilePathOf(user, id);
			const QByteArray content = readFile(path);
			Assert::IsTrue(QFile::remove(path));
			Assert::IsTrue(QDir().mkdir(path), L"a folder in its place: QFile::remove fails");
			Assert::IsTrue(_library->remove(id));
			_library->flush();
			Assert::IsTrue(QFileInfo(path).isDir());

			Assert::IsTrue(QDir().rmdir(path));
			writeFile(path, content); // "released": the file is still there
			_library->put(makeRecord("local", false, { "d2d4" }));
			_library->flush();
			Assert::IsFalse(QFile::exists(path), L"removed at the next write");
			makeLibrary();
			Assert::AreEqual(1, _library->count(), L"it doesn't come back");
		}

		// A games.json record whose id can't name a file gets a UUID made from it, the same each time: a
		// migration cut short by a failed write and tried again doesn't keep the game twice.
		TEST_METHOD(AMigrationTriedAgainDoesntKeepAGameTwice)
		{
			const QString odd = QStringLiteral("{9b2d1c4e-odd id}");
			const QString fileId = GameLibrary::fileIdOf(odd);
			Assert::IsTrue(GameLibrary::isFileId(fileId));
			Assert::AreEqual(fileId, GameLibrary::fileIdOf(odd), L"always the same");
			Assert::AreEqual(QStringLiteral("game-b"), GameLibrary::fileIdOf("game-b"), L"a good id stays");

			const QString user = addUser("Alice");
			GameRecord a = makeRecord("local", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-2));
			a.id = odd;
			GameRecord b2 = makeRecord("local", true, { "d2d4" }, QDateTime::currentDateTime().addDays(-1));
			b2.id = QStringLiteral("game-b");
			writeFile(GameLibrary::legacyFilePathOf(user), legacyContent({ a, b2 }));
			const QString path = GameLibrary::gameFilePathOf(user, fileId);
			Assert::IsTrue(QDir().mkpath(path), L"its file can't be written now");
			makeLibrary();
			Assert::IsTrue(QFile::exists(GameLibrary::legacyFilePathOf(user)), L"not migrated yet: kept");

			Assert::IsTrue(QDir().rmdir(path));
			_library->put(makeRecord("local", false, { "c2c4" }));
			_library->flush(); // the migration again
			Assert::AreEqual(3, _library->count(), L"its two games once, and the new one");
			Assert::IsTrue(_library->find(fileId).has_value());
			Assert::IsTrue(QFile::exists(GameLibrary::legacyFilePathOf(user) + ".migrated"));
			makeLibrary();
			Assert::AreEqual(3, _library->count());
		}

		// games.json (BL-19) had no ratedGames: the migration counts them back from its rating along the
		// rated records, oldest first. Records rated before BL-20 without them: counted as kept.
		TEST_METHOD(RatedGamesAreWorkedOutForOlderRecords)
		{
			const QString user = addUser("Alice");
			const QDateTime now = QDateTime::currentDateTime();
			GameRecord a = makeRecord("computer", true, { "e2e4", "e7e5" }, now.addDays(-3));
			a.id = QStringLiteral("game-a");
			a.rated = true; a.ratingBefore = 1300; a.ratingChange = 20;
			GameRecord b2 = makeRecord("computer", true, { "d2d4", "d7d5" }, now.addDays(-2));
			b2.id = QStringLiteral("game-b");
			b2.rated = true; b2.ratingBefore = 1320; b2.ratingChange = -10;
			GameRecord c = makeRecord("local", true, { "c2c4" }, now.addDays(-1));
			c.id = QStringLiteral("game-c");
			writeFile(GameLibrary::legacyFilePathOf(user), legacyContent({ a, b2, c }, 1310, 5)); // 3 rated games trimmed
			makeLibrary();
			Assert::AreEqual(4, _library->find("game-a")->ratedGames);
			Assert::AreEqual(5, _library->find("game-b")->ratedGames);
			Assert::AreEqual(0, _library->find("game-c")->ratedGames);
			Assert::IsFalse(_library->ratingRestored());
			Assert::AreEqual(1310, _library->rating());
			Assert::AreEqual(5, _library->ratedGames());

			a.ratedGames = b2.ratedGames = 0;
			const auto counted = GameLibrary::ratingFromGames({ b2, c, a });
			Assert::AreEqual(1310, counted->value(), L"the newest");
			Assert::AreEqual(2, counted->games(), L"the rated games kept");
			Assert::IsFalse(GameLibrary::ratingFromGames({ c }).has_value());
		}
	};
}
