#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <CppUnitTest.h>
#include "AppConfig.h"
#include "UserManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	inline std::wstring ToString(const QString& q) { return q.toStdWString(); }
}

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
