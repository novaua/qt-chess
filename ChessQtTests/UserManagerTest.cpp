#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <CppUnitTest.h>
#include "AppConfig.h"
#include "UserManager.h"
#include "gamelibrary.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// UserManager on a temporary data folder (AppConfig::setAppDataDir): users.json, backups,
// recovery from a damaged file, statistics, export and import.
namespace UserManagerTests
{
	static void writeFile(const QString& path, const QByteArray& data)
	{
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile f(path);
		Assert::IsTrue(f.open(QIODevice::WriteOnly), L"can't write a test file");
		f.write(data);
	}

	static QByteArray readFile(const QString& path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	static QString idOf(const UserManager& um, const QString& name)
	{
		for (const auto& u : um.users())
			if (u.toMap().value("name").toString() == name)
				return u.toMap().value("id").toString();
		return {};
	}

	static QStringList damagedFiles(const QString& dir)
	{
		return QDir(dir).entryList({ "users.damaged-*.json" }, QDir::Files);
	}

	// users.json with a profile's statistics edited by hand, its signature kept: a win vs the robot added.
	static void addAWinByHand(const QString& userId)
	{
		QJsonObject root = QJsonDocument::fromJson(readFile(UserManager::usersFilePath())).object();
		QJsonArray users = root["users"].toArray();
		for (int i = 0; i < users.size(); ++i) {
			QJsonObject u = users[i].toObject();
			if (u["id"].toString() != userId)
				continue;
			QJsonObject stats = u["stats"].toObject();
			QJsonObject byOpponent = stats["byOpponent"].toObject();
			QJsonObject robot = byOpponent[UserManager::computerKey(8)].toObject();
			robot["name"] = "Robot";
			robot["games"] = robot["games"].toInt() + 1;
			robot["wins"] = robot["wins"].toInt() + 1;
			byOpponent[UserManager::computerKey(8)] = robot;
			stats["byOpponent"] = byOpponent;
			u["stats"] = stats;
			users[i] = u;
		}
		root["users"] = users;
		writeFile(UserManager::usersFilePath(), QJsonDocument(root).toJson());
	}

	// A finished game kept by a profile (a signed file of its own).
	static void keepGame(const QString& userId, const QString& id, const QString& kind, const QString& result,
		const QString& opponentUserId = {}, const QString& opponentName = {}, bool resultRecorded = true)
	{
		GameRecord r;
		r.id = id;
		r.kind = kind;
		r.finished = true;
		r.startedAt = QDateTime::currentDateTime();
		r.finishedAt = r.startedAt.addSecs(60);
		r.result = result;
		r.level = kind == QLatin1String("computer") ? 3 : 0;
		r.opponentUserId = opponentUserId;
		r.opponentName = !opponentName.isEmpty() ? opponentName : kind == QLatin1String("computer") ? QString("Robot") : QString("Guest");
		r.resultRecorded = resultRecorded;
		r.moves = { "e2e4", "e7e5" };
		writeFile(GameLibrary::gameFilePathOf(userId, id), GameLibrary::gameFileContent(r));
	}

	TEST_CLASS(UserManagerTest)
	{
		std::unique_ptr<QTemporaryDir> _dir;

	public:
		TEST_METHOD_INITIALIZE(SetUp)
		{
			_dir = std::make_unique<QTemporaryDir>();
			AppConfig::setAppDataDir(_dir->path());
		}

		TEST_METHOD_CLEANUP(TearDown)
		{
			AppConfig::setAppDataDir({});
			_dir.reset();
		}

		// ── users.json ──────────────────────────────────────────────────────

		TEST_METHOD(Profiles_SurviveARestart)
		{
			{
				UserManager um;
				um.createUser("Alice", "girl");
				um.recordResult(um.activeUserId(), 1, UserManager::computerKey(3), "Robot");
			}
			UserManager um;
			Assert::AreEqual(1, um.userCount());
			Assert::AreEqual(QString("Alice"), um.activeUserName());
			Assert::AreEqual(1, um.gamesPlayed());
		}

		TEST_METHOD(Start_OneProfile_LoggedIn_SeveralAsk)
		{
			{
				UserManager um;
				um.createUser("Alice", "girl");
			}
			Assert::IsTrue(UserManager().isLoggedIn(), L"one profile: straight in");
			{
				UserManager um;
				um.createUser("Bob", "fox");
			}
			UserManager um;
			Assert::IsFalse(um.isLoggedIn(), L"two profiles: the profile list asks");
			Assert::AreEqual(idOf(um, "Bob"), um.activeUserId(), L"the last user is remembered");
		}

		TEST_METHOD(Statistics_PerOpponent_WinsDrawsLosses)
		{
			UserManager um;
			um.createUser("Bob", "fox");
			const QString bob = um.activeUserId();
			um.createUser("Alice", "girl");
			const QString alice = um.activeUserId();

			um.recordResult(alice, 1,  UserManager::userKey(bob), "Bob");
			um.recordResult(alice, 0,  UserManager::userKey(bob), "Bob");
			um.recordResult(alice, -1, UserManager::computerKey(2), "Robot");
			um.recordResult(bob,   -1, UserManager::userKey(alice), "Alice");

			const auto stats = um.statsOf(alice);
			const auto vsBob = stats.value(UserManager::userKey(bob));
			Assert::AreEqual(2, vsBob.games);
			Assert::AreEqual(1, vsBob.wins);
			Assert::AreEqual(1, vsBob.draws);
			Assert::AreEqual(0, vsBob.losses);
			Assert::AreEqual(1, stats.value(UserManager::computerKey(2)).losses);
			Assert::AreEqual(1, um.statsOf(bob).value(UserManager::userKey(alice)).losses);

			const QVariantMap totals = um.totals(); // Alice is active
			Assert::AreEqual(3, totals.value("games").toInt());
			const QVariantList rows = um.statistics();
			Assert::AreEqual(QString("computer"), rows.first().toMap().value("kind").toString(), L"the robot first");
		}

		TEST_METHOD(Statistics_Version1Totals_AreMigrated)
		{
			writeFile(UserManager::usersFilePath(), R"({ "version": 1, "activeUserId": "u1", "users": [
				{ "id": "u1", "name": "Old", "avatarName": "cat",
				  "stats": { "gamesPlayed": 7, "humanWins": 3, "computerWins": 2 } } ] })");
			UserManager um;
			const auto stats = um.statsOf("u1");
			const auto robot = stats.value(UserManager::computerKey(0));
			Assert::AreEqual(5, robot.games);
			Assert::AreEqual(3, robot.wins);
			Assert::AreEqual(2, robot.losses);
			Assert::AreEqual(2, stats.value("earlier").games, L"two-player games without a result");
			Assert::AreEqual(7, um.gamesPlayed());
		}

		// ── The statistics signed (BL-20 step 2) ──

		// users.json (version 3) signs each profile's statistics. Edited by hand, they fail their check: the
		// newest backup whose statistics for that profile verify gives them back, said once, written signed.
		TEST_METHOD(Statistics_Edited_RestoredFromTheNewestBackupThatVerifies)
		{
			QString alice;
			{
				UserManager um;
				um.createUser("Alice", "girl");
				alice = um.activeUserId();
				um.recordResult(alice, 1, UserManager::computerKey(3), "Robot");
			}
			{
				UserManager um; // the start copies users.json (one game) to backups/users-1.json
				um.recordResult(alice, -1, UserManager::computerKey(3), "Robot");
			}
			const QJsonObject root = QJsonDocument::fromJson(readFile(UserManager::usersFilePath())).object();
			Assert::AreEqual(UserManager::kUsersVersion, root["version"].toInt());
			Assert::IsTrue(root["users"].toArray()[0].toObject()["statsSig"].toString().startsWith("1:"), L"signed");

			addAWinByHand(alice);
			{
				UserManager um;
				const QVariantList restored = um.takeRestoredStatistics();
				Assert::AreEqual(1, (int)restored.size());
				Assert::AreEqual(QString("Alice"), restored[0].toMap()["name"].toString());
				Assert::IsFalse(restored[0].toMap()["rebuilt"].toBool(), L"from a backup");
				const auto stats = um.statsOf(alice);
				Assert::IsFalse(stats.contains(UserManager::computerKey(8)), L"the edit is gone");
				Assert::AreEqual(1, stats.value(UserManager::computerKey(3)).games, L"the backup's: the game after it is lost");
				Assert::AreEqual(1, stats.value(UserManager::computerKey(3)).wins);
				Assert::IsTrue(um.takeRestoredStatistics().isEmpty(), L"taken");
			}
			UserManager um;
			Assert::IsTrue(um.takeRestoredStatistics().isEmpty(), L"written signed again");
			Assert::AreEqual(1, um.gamesPlayed());
		}

		// No backup verifies: the statistics are rebuilt from the kept games that verify, the profile's own
		// and the two-player games another profile kept against it; Lichess and the edit are gone.
		TEST_METHOD(Statistics_NoBackupVerifies_RebuiltFromTheKeptGames)
		{
			QString alice, bob;
			{
				UserManager um;
				um.createUser("Bob", "fox");
				bob = um.activeUserId();
				um.createUser("Alice", "girl");
				alice = um.activeUserId();
				um.recordResult(alice, 1, "lichess", "Lichess");
			}
			keepGame(alice, "robot-win", "computer", "1-0");
			keepGame(alice, "guest-draw", "local", "1/2-1/2");
			keepGame(alice, "bob-loss", "local", "0-1", bob, "Bob");
			keepGame(alice, "not-counted", "computer", "1-0", {}, {}, false);
			addAWinByHand(alice);
			addAWinByHand(bob);
			QDir(QFileInfo(UserManager::backupFilePath(1)).absolutePath()).removeRecursively();

			UserManager um;
			const QVariantList restored = um.takeRestoredStatistics();
			Assert::AreEqual(2, (int)restored.size());
			Assert::IsTrue(restored[0].toMap()["rebuilt"].toBool());
			const auto stats = um.statsOf(alice);
			Assert::AreEqual(3, (int)stats.size(), L"robot, Guest, Bob: no Lichess, no edit");
			Assert::AreEqual(1, stats.value(UserManager::computerKey(3)).wins);
			Assert::AreEqual(1, stats.value(UserManager::computerKey(3)).games, L"only games the statistics had");
			Assert::AreEqual(1, stats.value("guest").draws);
			Assert::AreEqual(1, stats.value(UserManager::userKey(bob)).losses);
			const auto bobStats = um.statsOf(bob);
			Assert::AreEqual(1, (int)bobStats.size());
			Assert::AreEqual(1, bobStats.value(UserManager::userKey(alice)).wins, L"Alice's kept game, from Bob's side");
			Assert::AreEqual(QString("Alice"), bobStats.value(UserManager::userKey(alice)).opponentName);
		}

		// A users.json from before version 3 has no signatures: trusted once, signed at the next save.
		TEST_METHOD(Statistics_FromBeforeVersion3_TrustedOnceThenSigned)
		{
			writeFile(UserManager::usersFilePath(), R"({ "version": 2, "activeUserId": "u1", "users": [
				{ "id": "u1", "name": "Old", "avatarName": "cat",
				  "stats": { "byOpponent": { "computer:2": { "name": "Robot", "games": 4, "wins": 4, "draws": 0, "losses": 0 } } } } ] })");
			{
				UserManager um;
				Assert::IsTrue(um.takeRestoredStatistics().isEmpty());
				Assert::AreEqual(4, um.statsOf("u1").value(UserManager::computerKey(2)).wins);
				um.updateProfile("Old", "fox"); // any save
			}
			const QJsonObject root = QJsonDocument::fromJson(readFile(UserManager::usersFilePath())).object();
			Assert::AreEqual(UserManager::kUsersVersion, root["version"].toInt());
			UserManager um;
			Assert::IsTrue(um.takeRestoredStatistics().isEmpty(), L"they verify now");
			Assert::AreEqual(4, um.statsOf("u1").value(UserManager::computerKey(2)).wins);
		}

		TEST_METHOD(DeleteUser_RemovesProfileAndSaves_LogsOutTheActiveOne)
		{
			UserManager um;
			um.createUser("Alice", "girl");
			const QString alice = um.activeUserId();
			const QString save = AppConfig::autoSaveFilePath(alice);
			writeFile(save, "game");

			um.deleteUser(alice);
			Assert::AreEqual(0, um.userCount());
			Assert::IsFalse(um.isLoggedIn());
			Assert::IsFalse(QFile::exists(save), L"the profile's saves are deleted");
			Assert::AreEqual(0, UserManager().userCount(), L"deleted for good");
		}

		// ── Backups and recovery ────────────────────────────────────────────

		TEST_METHOD(Backups_RotateOnStart_KeepThree)
		{
			for (const char* name : { "A", "B", "C", "D" }) {
				UserManager um;          // start: copies the users.json it read
				um.createUser(name, "cat");
			}
			UserManager um;              // reads the file with A..D
			const QByteArray current = readFile(UserManager::usersFilePath());
			Assert::IsTrue(readFile(UserManager::backupFilePath(1)) == current, L"newest backup = users.json");
			Assert::IsTrue(QFile::exists(UserManager::backupFilePath(UserManager::kBackupCount)));
			Assert::IsFalse(QFile::exists(UserManager::backupFilePath(UserManager::kBackupCount + 1)));
			Assert::IsTrue(readFile(UserManager::backupFilePath(3)).contains("\"B\""), L"3rd newest: A and B");
			Assert::IsFalse(readFile(UserManager::backupFilePath(3)).contains("\"C\""));
		}

		TEST_METHOD(Backups_UnchangedFile_NotCopiedAgain)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
			}
			UserManager first;
			const QByteArray backup = readFile(UserManager::backupFilePath(1));
			UserManager second;
			Assert::IsTrue(readFile(UserManager::backupFilePath(1)) == backup);
			Assert::IsFalse(QFile::exists(UserManager::backupFilePath(2)), L"no duplicate copies");
		}

		TEST_METHOD(BackupNow_CopiesTheCurrentProfiles_OnceUntilTheyChange)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
			}
			UserManager um;              // start: backup 1 has A
			um.createUser("B", "fox");
			Assert::IsTrue(um.backupNow());
			Assert::IsTrue(readFile(UserManager::backupFilePath(1)) == readFile(UserManager::usersFilePath()));
			Assert::IsTrue(um.backupNow(), L"unchanged: nothing to do, still backed up");
			Assert::IsFalse(QFile::exists(UserManager::backupFilePath(3)), L"no duplicate copies");

			const QVariantList list = um.backups();
			Assert::AreEqual(2, (int)list.size());
			Assert::AreEqual(1, list[0].toMap().value("number").toInt());
			Assert::IsTrue(list[0].toMap().value("current").toBool());
			Assert::AreEqual(QStringList{ "A", "B" }.join(','), list[0].toMap().value("profiles").toStringList().join(','));
			Assert::AreEqual(QString("A"), list[1].toMap().value("profiles").toStringList().join(','));
			Assert::IsFalse(list[1].toMap().value("current").toBool());
		}

		TEST_METHOD(RestoreBackup_CurrentProfilesBackedUpFirst_ActiveUserStays)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
			}
			UserManager um;              // backup 1: A
			const QString a = um.activeUserId();
			um.createUser("B", "fox");
			um.login(a);
			um.recordResult(a, 1, UserManager::computerKey(3), "Robot");

			Assert::IsTrue(um.restoreBackup(1));
			Assert::AreEqual(1, um.userCount(), L"only A, as in the backup");
			Assert::IsTrue(um.isLoggedIn());
			Assert::AreEqual(a, um.activeUserId());
			Assert::AreEqual(0, um.gamesPlayed(), L"A's statistics from the backup");

			const QVariantList list = um.backups();
			Assert::AreEqual(2, (int)list.size());
			Assert::AreEqual(QString("A,B"), list[0].toMap().value("profiles").toStringList().join(','),
				L"what was there before the restore is now the newest backup");
			// Read the file: another UserManager would back it up on start and shift the backups.
			Assert::IsFalse(readFile(UserManager::usersFilePath()).contains("\"B\""), L"users.json restored on disk");

			Assert::IsTrue(um.restoreBackup(1), L"and the restore can be undone");
			Assert::AreEqual(2, um.userCount());
			Assert::AreEqual(1, um.gamesPlayed());
		}

		// The data folder copied to another PC or Windows account: the Lichess token, encrypted for the
		// account that linked it, can't be decrypted. The profile asks to reconnect instead of saying
		// Connected with no token behind it; the rest of the profile is as it was.
		TEST_METHOD(LichessToken_CantBeDecryptedHere_AsksToReconnect)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
				um.saveLichessCredentials("token-123", "a_li");
				Assert::AreEqual(QString("token-123"), um.lichessToken(), L"decrypted on the account that linked it");
			}
			// Another account's encryption: bytes this one can't decrypt.
			QByteArray users = readFile(UserManager::usersFilePath());
			const QJsonObject before = QJsonDocument::fromJson(users).object();
			const QString encrypted = before.value("users").toArray().first().toObject().value("lichessToken").toString();
			Assert::IsFalse(encrypted.isEmpty());
			users.replace(encrypted.toLatin1(), QByteArray("not-this-accounts-token").toBase64());
			writeFile(UserManager::usersFilePath(), users);

			UserManager um;
			Assert::IsTrue(um.lichessConnected(), L"still says linked until the token is read");
			QSignalSpy changed(&um, &UserManager::lichessChanged);
			Assert::IsTrue(um.lichessToken().isEmpty());
			Assert::IsFalse(um.lichessConnected());
			Assert::IsTrue(um.lichessNeedsReconnect(), L"reconnect in your profile");
			Assert::AreEqual(QString("a_li"), um.lichessUsername(), L"the account it was linked to is kept");
			Assert::AreEqual(1, (int)changed.count());
			Assert::IsTrue(um.lichessToken().isEmpty(), L"asked again: still nothing, no second change");
			Assert::AreEqual(1, (int)changed.count());
		}

		TEST_METHOD(RestoreBackup_KeepsThisPcsLichessSignIn_AndCountsAsTheSameAsNow)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
				um.saveLichessCredentials("token", "a_li");
				um.createUser("B", "fox");   // active: B
			}
			UserManager um;                  // backup 1: A (linked) and B, B active
			const QString a = idOf(um, "A");
			um.login(a);
			um.clearLichessToken();          // signed out of Lichess since the backup
			um.recordResult(a, 1, UserManager::computerKey(3), "Robot");

			Assert::IsTrue(um.restoreBackup(1));
			Assert::AreEqual(a, um.activeUserId(), L"A stays logged in, not the backup's B");
			Assert::AreEqual(0, um.gamesPlayed(), L"statistics from the backup");
			Assert::IsFalse(um.lichessConnected(), L"the removed sign-in doesn't come back");

			const QVariantList list = um.backups();
			Assert::AreEqual(2, (int)list.size());
			Assert::IsFalse(list[0].toMap().value("current").toBool(), L"before the restore: A's win");
			Assert::IsFalse(list[1].toMap().value("current").toBool(), L"the backup still has the Lichess link");
		}

		TEST_METHOD(RestoreBackup_ActiveProfileKept_TheBackupIsTheSameAsNow)
		{
			{
				UserManager um;
				um.createUser("A", "cat");
				um.createUser("B", "fox");   // active: B
			}
			UserManager um;                  // backup 1: A and B, B active
			um.login(idOf(um, "A"));
			um.recordResult(um.activeUserId(), 1, UserManager::computerKey(3), "Robot");

			Assert::IsTrue(um.restoreBackup(1));
			const QVariantList list = um.backups();
			Assert::AreEqual(2, (int)list.size());
			Assert::IsTrue(list[1].toMap().value("current").toBool(),
				L"the restored backup is the same profiles, though A (not B) is logged in");
		}

		TEST_METHOD(RestoreBackup_MissingOrDamaged_Refused)
		{
			UserManager um;
			um.createUser("A", "cat");
			Assert::IsFalse(um.restoreBackup(1), L"no backup yet");
			Assert::IsFalse(um.restoreBackup(0));
			writeFile(UserManager::backupFilePath(2), "broken");
			Assert::IsFalse(um.restoreBackup(2));
			Assert::AreEqual(1, um.userCount());
			Assert::IsTrue(um.backups().isEmpty(), L"a backup that doesn't parse isn't offered");
		}

		TEST_METHOD(DamagedFile_NothingLoaded_RestoreFromBackup)
		{
			{
				UserManager um;
				um.createUser("Alice", "girl");
			}
			{ UserManager um; } // backs it up
			writeFile(UserManager::usersFilePath(), "{ \"users\": [ broken");

			UserManager um;
			Assert::IsTrue(um.dataDamaged());
			Assert::IsTrue(um.canRestoreBackup());
			Assert::AreEqual(0, um.userCount());

			Assert::IsTrue(um.restoreFromBackup());
			Assert::IsFalse(um.dataDamaged());
			Assert::AreEqual(1, um.userCount());
			Assert::AreEqual(QString("Alice"), um.users().first().toMap().value("name").toString());
			Assert::AreEqual(1, (int)damagedFiles(_dir->path()).size(), L"the damaged file is kept");
			Assert::AreEqual(1, UserManager().userCount(), L"users.json restored on disk");
		}

		TEST_METHOD(DamagedFile_KeptWhenTheUserStartsOver)
		{
			writeFile(UserManager::usersFilePath(), "not json");
			UserManager um;
			Assert::IsTrue(um.dataDamaged());
			Assert::IsFalse(um.canRestoreBackup(), L"no backup yet");
			Assert::AreEqual(0, (int)damagedFiles(_dir->path()).size(), L"untouched until a save");

			um.createUser("New", "cat");
			Assert::IsFalse(um.dataDamaged());
			const QStringList kept = damagedFiles(_dir->path());
			Assert::AreEqual(1, (int)kept.size());
			Assert::IsTrue(readFile(_dir->path() + "/" + kept.first()) == "not json");
		}

		TEST_METHOD(DamagedFile_BackupWithoutUsers_NoRestoreOffered)
		{
			writeFile(UserManager::backupFilePath(1), R"({ "version": 2 })");
			writeFile(UserManager::usersFilePath(), "broken");
			UserManager um;
			Assert::IsTrue(um.dataDamaged());
			Assert::IsFalse(um.canRestoreBackup(), L"Restore would fail: no users array");
			Assert::IsFalse(um.restoreFromBackup());
			Assert::IsTrue(readFile(UserManager::usersFilePath()) == "broken", L"untouched");
		}

		TEST_METHOD(AutoSaveInfo_KeepsRobotLevelAndCountedResult)
		{
			{
				UserManager um;
				um.createUser("Alice", "girl");
				UserManager::GameSaveInfo info;
				info.isSinglePlayer = true;
				info.computerLevel  = 5;
				info.resultRecorded = true;
				info.clockLimitSec  = 180;
				info.clockIncSec    = 2;
				info.whiteMs        = 95000;
				info.blackMs        = 62000;
				um.setAutoSaveInfo(info);
			}
			const auto info = UserManager().autoSaveInfo();
			Assert::AreEqual(5, info.computerLevel);
			Assert::IsTrue(info.resultRecorded);
			Assert::AreEqual(180, info.clockLimitSec);
			Assert::AreEqual(2, info.clockIncSec);
			Assert::AreEqual(qint64(95000), info.whiteMs);
			Assert::AreEqual(qint64(62000), info.blackMs);
		}

		// ── Export / import ─────────────────────────────────────────────────

		TEST_METHOD(Export_HasProfileAndSaves_NeverTheToken)
		{
			UserManager um;
			um.createUser("Alice", "girl");
			const QString alice = um.activeUserId();
			um.saveLichessCredentials("secret-token-123", "alice_li");
			writeFile(AppConfig::autoSaveFilePath(alice), "autosave-bytes");
			const QString file = _dir->path() + "/Alice.chessprofile";

			Assert::AreEqual(QString(), um.exportProfile(alice, QUrl::fromLocalFile(file)));
			const QByteArray text = readFile(file);
			Assert::IsFalse(text.contains("lichessToken"), L"no token field");
			Assert::IsFalse(text.contains(um.lichessTokenEncrypted().toUtf8()), L"no encrypted token");
			const QJsonObject root = QJsonDocument::fromJson(text).object();
			Assert::AreEqual(QString("alice_li"), root["profile"].toObject()["lichessUsername"].toString());
			Assert::IsTrue(QByteArray::fromBase64(root["files"].toObject()["chess.autosave"].toString().toLatin1())
				== "autosave-bytes");
		}

		TEST_METHOD(Import_OnAnotherPc_AddsProfileWithSaves_AsksToReconnectLichess)
		{
			const QString file = _dir->path() + "/export/Alice.chessprofile";
			QDir().mkpath(_dir->path() + "/export");
			{
				UserManager um;
				um.createUser("Alice", "girl");
				um.saveLichessCredentials("token", "alice_li");
				um.recordResult(um.activeUserId(), 1, "guest", "Guest");
				writeFile(AppConfig::savedGameFilePath(um.activeUserId()), "saved-bytes");
				Assert::AreEqual(QString(), um.exportProfile(um.activeUserId(), QUrl::fromLocalFile(file)));
			}
			QTemporaryDir otherPc;
			AppConfig::setAppDataDir(otherPc.path());

			UserManager um;
			const QVariantMap info = um.inspectImport(QUrl::fromLocalFile(file));
			Assert::AreEqual(QString(), info["error"].toString());
			Assert::AreEqual(QString("Alice"), info["name"].toString());
			Assert::AreEqual(QString(), info["conflictId"].toString());

			Assert::AreEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), false));
			Assert::AreEqual(1, um.userCount());
			const QString id = idOf(um, "Alice");
			um.login(id);
			Assert::AreEqual(1, um.gamesPlayed());
			Assert::IsFalse(um.lichessConnected());
			Assert::IsTrue(um.lichessNeedsReconnect());
			Assert::IsTrue(readFile(AppConfig::savedGameFilePath(id)) == "saved-bytes");
		}

		// BL-20: an export carries the profile's games as they are (one signed file each) and its rating;
		// on the other PC they verify (one app-wide key). Nothing but games/<id>.json is written under games/.
		TEST_METHOD(ExportImport_TheSignedGamesAndTheRatingVerifyOnAnotherPc)
		{
			const QString file = _dir->path() + "/export/Alice.chessprofile";
			QDir().mkpath(_dir->path() + "/export");
			GameRecord game;
			game.id = QStringLiteral("0b0f6c5e-1a2b-4c3d-9e8f-0123456789ab");
			game.kind = QStringLiteral("local");
			game.opponentName = QStringLiteral("Guest");
			game.startedAt = QDateTime(QDate(2026, 11, 18), QTime(18, 0));
			game.moves = { "e2e4", "e7e5" };
			{
				UserManager um;
				um.createUser("Alice", "girl");
				const QString alice = um.activeUserId();
				writeFile(GameLibrary::gameFilePathOf(alice, game.id), GameLibrary::gameFileContent(game));
				writeFile(GameLibrary::ratingFilePathOf(alice), GameLibrary::ratingFileContent(LocalRating(1250, 3)));
				writeFile(GameLibrary::gamesDirOf(alice) + "/notes.txt", "not a game");
				Assert::AreEqual(QString(), um.exportProfile(alice, QUrl::fromLocalFile(file)));
			}
			QTemporaryDir otherPc;
			AppConfig::setAppDataDir(otherPc.path());

			UserManager um;
			Assert::AreEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), false));
			const QString id = idOf(um, "Alice");
			const auto read = GameLibrary::readGameFile(GameLibrary::gameFilePathOf(id, game.id));
			Assert::IsTrue(read.has_value(), L"the game verifies on the other PC");
			Assert::AreEqual(QStringLiteral("e2e4 e7e5"), read->moves.join(' '));
			const auto rating = GameLibrary::readRatingFile(GameLibrary::ratingFilePathOf(id));
			Assert::IsTrue(rating.has_value(), L"the rating verifies on the other PC");
			Assert::AreEqual(1250, rating->value());
			Assert::IsFalse(QFile::exists(GameLibrary::gamesDirOf(id) + "/notes.txt"), L"only game files");
		}

		// An export naming a file outside the profile's games (a path in the name) writes nothing of it.
		TEST_METHOD(Import_OnlyGameFileNamesAreWritten)
		{
			const QString file = _dir->path() + "/Evil.chessprofile";
			const QJsonObject root{
				{ "format", "chessplusplus-profile" },
				{ "version", 2 },
				{ "profile", QJsonObject{ { "id", "evil" }, { "name", "Evil" }, { "avatar", "fox" } } },
				{ "files", QJsonObject{
					{ "games/../../escape.json", QString::fromLatin1(QByteArray("x").toBase64()) },
					{ "games/sub/inner.json",    QString::fromLatin1(QByteArray("x").toBase64()) },
					{ "games/ok-1.json",         QString::fromLatin1(QByteArray("x").toBase64()) },
					{ "games/ok-2xjson",         QString::fromLatin1(QByteArray("x").toBase64()) },
				} },
			};
			writeFile(file, QJsonDocument(root).toJson());
			UserManager um;
			Assert::AreEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), false));
			const QString id = idOf(um, "Evil");
			Assert::IsFalse(QFile::exists(_dir->path() + "/users/escape.json"), L"not out of the profile's folder");
			Assert::IsFalse(QFile::exists(GameLibrary::gamesDirOf(id) + "/sub/inner.json"));
			Assert::IsFalse(QFile::exists(GameLibrary::gamesDirOf(id) + "/ok-2xjson"), L"\".json\", not any character");
			Assert::IsTrue(QFile::exists(GameLibrary::gameFilePathOf(id, "ok-1")), L"a game file's name is written");
		}

		TEST_METHOD(Import_Conflict_AsNewProfile_GetsSuffix)
		{
			UserManager um;
			um.createUser("Bob", "fox");
			const QString bob = um.activeUserId();
			const QString file = _dir->path() + "/Bob.chessprofile";
			um.exportProfile(bob, QUrl::fromLocalFile(file));

			Assert::AreEqual(bob, um.inspectImport(QUrl::fromLocalFile(file))["conflictId"].toString());
			Assert::AreEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), false));
			Assert::AreEqual(2, um.userCount());
			const QString copy = idOf(um, "Bob (2)");
			Assert::IsFalse(copy.isEmpty(), L"named Bob (2)");
			Assert::AreNotEqual(bob, copy, L"a new id");
		}

		TEST_METHOD(Import_Conflict_Replace_OverwritesTheProfile)
		{
			UserManager um;
			um.createUser("Bob", "fox");
			const QString bob = um.activeUserId();
			const QString file = _dir->path() + "/Bob.chessprofile";
			um.exportProfile(bob, QUrl::fromLocalFile(file));
			um.recordResult(bob, 1, "guest", "Guest"); // after the export
			um.logout();

			Assert::AreEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), true));
			Assert::AreEqual(1, um.userCount());
			Assert::AreEqual(0, um.statsOf(bob).value("guest").games, L"the exported statistics");
		}

		TEST_METHOD(Import_Replace_ActiveProfile_Refused)
		{
			UserManager um;
			um.createUser("Bob", "fox");
			const QString file = _dir->path() + "/Bob.chessprofile";
			um.exportProfile(um.activeUserId(), QUrl::fromLocalFile(file));
			Assert::AreNotEqual(QString(), um.importProfile(QUrl::fromLocalFile(file), true));
		}

		TEST_METHOD(Import_NotAProfileOrNewerVersion_Refused)
		{
			UserManager um;
			const QString notProfile = _dir->path() + "/other.json";
			writeFile(notProfile, R"({ "users": [] })");
			Assert::AreNotEqual(QString(), um.inspectImport(QUrl::fromLocalFile(notProfile))["error"].toString());

			const QString newer = _dir->path() + "/newer.chessprofile";
			writeFile(newer, R"({ "format": "chessplusplus-profile", "version": 99,
				"profile": { "id": "x", "name": "Future" } })");
			Assert::AreNotEqual(QString(), um.importProfile(QUrl::fromLocalFile(newer), false));
			Assert::AreEqual(0, um.userCount());
		}

		TEST_METHOD(ExportFileName_StripsCharactersWindowsRefuses)
		{
			UserManager um;
			um.createUser("A/B: \"C\"?", "cat");
			Assert::AreEqual(QString("A_B_ _C__.chessprofile"), um.exportFileName(um.activeUserId()));
		}
	};
}
