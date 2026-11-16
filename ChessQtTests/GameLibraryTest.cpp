#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <CppUnitTest.h>
#include "AppConfig.h"
#include "Game.h"
#include "UserManager.h"
#include "gamelibrary.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// The games library (BL-19) on a temporary data folder: records kept per profile in games.json,
// the cap on finished games, the migration of the two save slots of earlier versions, a damaged file.
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
			Assert::IsFalse(QFile::exists(GameLibrary::filePathOf(user)), L"written once the event is done, not at once");
			QCoreApplication::processEvents();
			Assert::IsTrue(QFile::exists(GameLibrary::filePathOf(user)), L"games.json written");

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
			Assert::IsTrue(QFile::exists(GameLibrary::filePathOf(bob)));
			_users->login(bob);
			_users->deleteUser(bob);
			Assert::IsFalse(QFile::exists(GameLibrary::filePathOf(bob)), L"the profile's folder went with it");
			Assert::AreEqual(0, _library->count());
		}

		TEST_METHOD(ADamagedFileIsKeptAsideAndTheLibraryStartsEmpty)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("local", false, { "e2e4" }));
			_library->flush();
			QFile f(GameLibrary::filePathOf(user));
			Assert::IsTrue(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
			f.write("{ not json");
			f.close();
			makeLibrary();
			Assert::AreEqual(0, _library->count());
			Assert::IsTrue(QFile::exists(GameLibrary::filePathOf(user) + ".damaged"), L"kept aside");
			_library->put(makeRecord("local", false, { "d2d4" }));
			makeLibrary();
			Assert::AreEqual(1, _library->count(), L"a fresh file works");
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
			QFile::remove(GameLibrary::filePathOf(user)); // no games.json yet: the first load migrates
			makeLibrary();
			Assert::AreEqual(2, _library->count());
			Assert::IsFalse(QFile::exists(AppConfig::autoSaveFilePath(user)), L"the autosave file went");
			Assert::IsFalse(QFile::exists(AppConfig::savedGameFilePath(user)), L"the save file went");
			Assert::IsTrue(QFile::exists(GameLibrary::filePathOf(user)), L"games.json written");
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
			QFile::remove(GameLibrary::filePathOf(user));
			makeLibrary();
			Assert::AreEqual(1, _library->count());
			Assert::IsFalse(_library->records()[0].finished, L"not over: Continue offers it");
			Assert::AreEqual(QStringLiteral("local"), _library->records()[0].kind);
		}

		TEST_METHOD(TheFileIsCompactJsonWithAVersion)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("local", false, { "e2e4" }));
			_library->flush();
			const QJsonObject root = QJsonDocument::fromJson(readFile(GameLibrary::filePathOf(user))).object();
			Assert::AreEqual(GameLibrary::FormatVersion, root.value("version").toInt());
			Assert::AreEqual(1, (int)root.value("games").toArray().size());
			const QJsonObject g = root.value("games").toArray()[0].toObject();
			Assert::AreEqual(QStringLiteral("e2e4"), g.value("moves").toArray()[0].toString());
			Assert::IsFalse(g.contains("result"), L"no result while unfinished");
			Assert::IsFalse(g.contains("analysis"), L"nothing until analysed");
		}
		// A games.json that is there but can't be read now (here: a folder of that name) is never written
		// over; once it can be read, its games and the ones kept meanwhile go in together (review #3).
		TEST_METHOD(AFileThatCantBeReadIsntWrittenOver)
		{
			const QString user = addUser("Alice");
			_library->put(makeRecord("local", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-2)));
			_library->flush();
			const QByteArray before = readFile(GameLibrary::filePathOf(user));
			Assert::IsTrue(QFile::rename(GameLibrary::filePathOf(user), GameLibrary::filePathOf(user) + ".held"));
			Assert::IsTrue(QDir().mkdir(GameLibrary::filePathOf(user)), L"a folder in its place: it can't be opened");
			makeLibrary();
			Assert::AreEqual(0, _library->count(), L"its games can't be shown");
			_library->put(makeRecord("local", false, { "d2d4" }));
			_library->flush();
			Assert::IsTrue(QFileInfo(GameLibrary::filePathOf(user)).isDir(), L"not written over");

			// It can be read again: the next write has both.
			Assert::IsTrue(QDir().rmdir(GameLibrary::filePathOf(user)));
			Assert::IsTrue(QFile::rename(GameLibrary::filePathOf(user) + ".held", GameLibrary::filePathOf(user)));
			_library->put(makeRecord("local", false, { "c2c4" }));
			_library->flush();
			Assert::AreEqual(3, _library->count(), L"the file's game and the two kept meanwhile");
			makeLibrary();
			Assert::AreEqual(3, _library->count());
			Assert::AreNotEqual(before.size(), readFile(GameLibrary::filePathOf(user)).size());
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
			QFile::remove(GameLibrary::filePathOf(user));
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
			const QString id = _library->put(makeRecord("computer", true, { "e2e4" })); // White (the player) won
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

			GameRecord lost = makeRecord("computer", true, { "d2d4" });
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
			addUser("Alice");
			_library->put(makeRecord("local", true, { "e2e4" }));
			_library->put(makeRecord("computer", false, { "e2e4" }));
			GameRecord undone = makeRecord("computer", true, { "e2e4" });
			undone.undoCount = 1;
			_library->put(undone);
			GameRecord fork = makeRecord("computer", true, { "e2e4" });
			fork.forkOfId = QStringLiteral("other");
			fork.forkPly = 1;
			_library->put(fork);
			Assert::AreEqual(1200, _library->rating());
			Assert::AreEqual(0, _library->ratedGames());
			for (const auto& r : _library->records())
				Assert::IsFalse(r.rated);
			_library->flush();
			const QJsonObject root = QJsonDocument::fromJson(readFile(_library->filePath())).object();
			Assert::IsFalse(root.contains("rating"), L"no rating in the file until a game is rated");
		}

		// A takeback after the result resumes the game: its change goes back, and with a takeback it is
		// never rated again.
		TEST_METHOD(ATakebackGivesTheChangeBack)
		{
			addUser("Alice");
			_library->put(makeRecord("computer", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-1)));
			const QString id = _library->put(makeRecord("computer", true, { "e2e4", "e7e5" }));
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

		// Removing games, by hand or by the cap, never changes the rating; another profile's file keeps
		// its own when the cap trims it.
		TEST_METHOD(RemovingGamesKeepsTheRating)
		{
			const QString alice = addUser("Alice");
			for (int i = 0; i < 3; ++i)
				_library->put(makeRecord("computer", true, { "e2e4" }, QDateTime::currentDateTime().addDays(-10 + i)));
			const int aliceRating = _library->rating();
			Assert::AreEqual(3, _library->ratedGames());
			const QString bob = addUser("Bob");
			Assert::AreEqual(1200, _library->rating(), L"Bob's own");
			const QString lost = _library->put(makeRecord("computer", true, { "d2d4" }));
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
	};
}
