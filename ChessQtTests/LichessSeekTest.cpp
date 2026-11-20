#include <QUrlQuery>
#include <CppUnitTest.h>
#include "LichessClient.h"
#include "TestSupport.h"
#include "FakeLichess.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// LI-09 step 1: quick pairing's seek (POST /api/board/seek) against FakeSeekLichess.
namespace LichessTests
{
	TEST_CLASS(LichessSeekTests)
	{
		// What a test sees of the client.
		struct Watch
		{
			QStringList started; // gameStarted's ids
			QStringList failed;  // seekFailed's messages
			QStringList sent;    // seekSent's messages
			int         changes = 0;

			explicit Watch(LichessClient& client)
			{
				QObject::connect(&client, &LichessClient::gameStarted, [this](QString id) { started << id; });
				QObject::connect(&client, &LichessClient::seekFailed, [this](QString message) { failed << message; });
				QObject::connect(&client, &LichessClient::seekSent, [this](QString message) { sent << message; });
				QObject::connect(&client, &LichessClient::seekChanged, [this]() { ++changes; });
			}
		};

		static void setUp(LichessClient& client, const FakeLichess& lichess)
		{
			client.setUsername("me");
			client.setBaseUrl(lichess.url());
			client.setStreamTiming(5000, 100, 5000, 300);
			client.setSeekTiming(1000, 100);
			client.setToken("token");
		}

		// A 10+0 seek, open on the fake with the event stream.
		static void seek(LichessClient& client, FakeSeekLichess& lichess, bool rated = true)
		{
			client.startSeek(600, 0, 0, rated, "standard");
			waitUntil([&] { return lichess.seekSocket != nullptr && lichess.events != nullptr; });
			Assert::IsNotNull(lichess.seekSocket, L"the seek's request is open");
		}

		static QUrlQuery seekBody(const FakeLichess& lichess)
		{
			return QUrlQuery(lichess.bodies.value(int(lichess.requests.lastIndexOf(FakeSeekLichess::Seek))));
		}

	public:
		TEST_METHOD(SpeedOf_LichessRule)
		{
			Assert::AreEqual(QString("ultraBullet"), LichessClient::speedOf(15, 0));
			Assert::AreEqual(QString("bullet"), LichessClient::speedOf(60, 0));
			Assert::AreEqual(QString("blitz"), LichessClient::speedOf(300, 3), L"420 s");
			Assert::AreEqual(QString("blitz"), LichessClient::speedOf(479, 0));
			Assert::AreEqual(QString("rapid"), LichessClient::speedOf(480, 0));
			Assert::AreEqual(QString("rapid"), LichessClient::speedOf(300, 5), L"500 s");
			Assert::AreEqual(QString("rapid"), LichessClient::speedOf(30, 12), L"½ + 12: 510 s");
			Assert::AreEqual(QString("rapid"), LichessClient::speedOf(900, 10));
			Assert::AreEqual(QString("classical"), LichessClient::speedOf(1800, 0));
			Assert::AreEqual(QString("classical"), LichessClient::speedOf(1500, 0));
		}

		// Someone takes it: Lichess's gameStart (source lobby) starts the game, and the seek is over.
		TEST_METHOD(Seek_Paired_TheGameStarts)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			const QUrlQuery body = seekBody(lichess);
			Assert::AreEqual(QString("10"), body.queryItemValue("time"));
			Assert::AreEqual(QString("0"), body.queryItemValue("increment"));
			Assert::AreEqual(QString("true"), body.queryItemValue("rated"));
			Assert::AreEqual(QString("standard"), body.queryItemValue("variant"));
			Assert::IsFalse(body.hasQueryItem("days"));
			const QVariantMap state = client.seekState();
			Assert::AreEqual(600, state.value("limitSec").toInt());
			Assert::AreEqual(QString("rapid"), state.value("speed").toString());
			Assert::IsTrue(state.value("rated").toBool());
			Assert::IsTrue(state.value("since").toLongLong() > 0);
			Assert::AreEqual(1, watch.changes);

			lichess.pair("seek0001");
			waitUntil([&] { return !watch.started.isEmpty(); });

			Assert::AreEqual(QString("seek0001"), watch.started.join(","));
			Assert::IsFalse(client.seeking());
			Assert::AreEqual(2, watch.changes);
			Assert::IsTrue(watch.failed.isEmpty());
			Assert::AreEqual(1, lichess.count("GET /api/board/game/stream/seek0001"));
		}

		// A pool player paired with it: also the seek's.
		TEST_METHOD(Seek_PairedFromThePool_TheGameStarts)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			lichess.pair("pool0001", "pool");
			waitUntil([&] { return !watch.started.isEmpty(); });

			Assert::AreEqual(QString("pool0001"), watch.started.join(","));
		}

		TEST_METHOD(Seek_FractionOfAMinute_CasualChess960)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);

			client.startSeek(30, 12, 0, false, "chess960");
			waitUntil([&] { return lichess.seekSocket != nullptr; });

			const QUrlQuery body = seekBody(lichess);
			Assert::AreEqual(QString("0.5"), body.queryItemValue("time"));
			Assert::AreEqual(QString("12"), body.queryItemValue("increment"));
			Assert::AreEqual(QString("false"), body.queryItemValue("rated"));
			Assert::AreEqual(QString("chess960"), body.queryItemValue("variant"));
			client.cancelSeek();
		}

		// Cancel closes the request (Lichess drops the seek), at once and quietly.
		TEST_METHOD(Cancel_ClosesTheConnection)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			client.cancelSeek();
			Assert::IsFalse(client.seeking());
			Assert::AreEqual(2, watch.changes);
			waitUntil([&] { return lichess.dropped.contains(FakeSeekLichess::Seek); });
			waitUntil([] { return false; }, 300);

			Assert::IsTrue(lichess.dropped.contains(FakeSeekLichess::Seek), L"the fake saw the close");
			Assert::IsTrue(watch.failed.isEmpty());
			Assert::IsTrue(watch.started.isEmpty());
		}

		// One seek at a time: a new one closes the last.
		TEST_METHOD(SecondSeek_ReplacesTheFirst)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			QTcpSocket* first = lichess.seekSocket;
			client.startSeek(900, 10, 0, true, "standard");
			waitUntil([&] { return lichess.count(FakeSeekLichess::Seek) == 2 && lichess.seekSocket && lichess.seekSocket != first; });
			waitUntil([&] { return lichess.dropped.contains(FakeSeekLichess::Seek); });

			Assert::AreEqual(1, int(lichess.dropped.count(FakeSeekLichess::Seek)));
			Assert::AreEqual(900, client.seekState().value("limitSec").toInt());
			Assert::IsTrue(watch.failed.isEmpty());
			client.cancelSeek();
		}

		// Lichess's refusal is a message, not a silent stop: its own words, or for a 429 ours.
		TEST_METHOD(Refused_400And429_Reported)
		{
			for (int status : { 400, 429 }) {
				FakeSeekLichess lichess;
				lichess.seekStatus = status;
				LichessClient client;
				setUp(client, lichess);
				Watch watch(client);

				client.startSeek(300, 0, 0, true, "standard");
				waitUntil([&] { return !watch.failed.isEmpty(); });

				Assert::AreEqual(1, int(watch.failed.size()));
				Assert::AreEqual(status == 400 ? QString("Invalid speed") : QString("Lichess asks to wait a minute before seeking again"),
				                 watch.failed[0]);
				Assert::IsFalse(client.seeking());
				Assert::AreEqual(2, watch.changes, L"seeking, then not");
			}
		}

		// Lichess closed the seek and no game came (expired): said so once the fallback gives up.
		TEST_METHOD(EndedUnpaired_Reported)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			lichess.endSeek();
			waitUntil([&] { return !watch.failed.isEmpty(); });

			Assert::AreEqual(QString("No opponent was found: Lichess ended the seek."), watch.failed.join(","));
			Assert::IsFalse(client.seeking());
			Assert::IsTrue(lichess.count("GET /api/account/playing?nb=50") >= 2, L"Your games asked more than once");
			Assert::IsTrue(watch.started.isEmpty());
		}

		// The gameStart was missed (the event stream reconnecting): Lichess closed the seek, and the new
		// real-time game in Your games is its game. One that was there before the seek isn't.
		TEST_METHOD(Fallback_TheNewGameInYourGames)
		{
			FakeSeekLichess lichess;
			lichess.playing = "{\"nowPlaying\":[" + FakeSeekLichess::playingGame("old00001") + "]}";
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);
			client.refreshOngoingGames();
			waitUntil([&] { return client.ongoingGamesLoaded() && !client.ongoingGamesLoading(); });

			seek(client, lichess);
			lichess.playing = "{\"nowPlaying\":[" + FakeSeekLichess::playingGame("old00001") + ","
				+ FakeSeekLichess::playingGame("corr0002", "correspondence") + ","
				+ FakeSeekLichess::playingGame("seek0002") + "]}";
			lichess.endSeek();
			waitUntil([&] { return !watch.started.isEmpty() || !watch.failed.isEmpty(); });

			Assert::AreEqual(QString("seek0002"), watch.started.join(","));
			Assert::IsTrue(watch.failed.isEmpty());
			Assert::IsFalse(client.seeking());
		}

		// Lichess closes the seek before its gameStart comes: the gameStart still starts the game.
		TEST_METHOD(ClosedFirst_ThenGameStart_TheGameStarts)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			lichess.endSeek();
			waitUntil([&] { return lichess.count("GET /api/account/playing?nb=50") >= 1; });
			FakeLichess::line(lichess.events, FakeSeekLichess::gameStart("seek0003", "lobby"));
			waitUntil([&] { return !watch.started.isEmpty(); });
			waitUntil([] { return false; }, 1200); // longer than the fallback

			Assert::AreEqual(QString("seek0003"), watch.started.join(","));
			Assert::IsTrue(watch.failed.isEmpty());
		}

		// Not the seek's: a challenge's game (source friend), another speed, a game going on before the
		// seek (resent when the event stream reconnects). And leaving a game (stopStream) keeps the seek.
		TEST_METHOD(OtherGamesStart_NotTaken)
		{
			FakeSeekLichess lichess;
			lichess.playing = "{\"nowPlaying\":[" + FakeSeekLichess::playingGame("old00001") + "]}";
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);
			client.refreshOngoingGames();
			waitUntil([&] { return client.ongoingGamesLoaded() && !client.ongoingGamesLoading(); });

			seek(client, lichess);
			FakeLichess::line(lichess.events, FakeSeekLichess::gameStart("frnd0001", "friend"));
			FakeLichess::line(lichess.events, FakeSeekLichess::gameStart("clas0001", "lobby", "classical"));
			FakeLichess::line(lichess.events, FakeSeekLichess::gameStart("old00001", "lobby"));
			client.stopStream();
			waitUntil([] { return false; }, 300);

			Assert::IsTrue(watch.started.isEmpty());
			Assert::IsTrue(client.seeking());
			Assert::IsTrue(lichess.dropped.isEmpty(), L"the seek's request is still open");

			lichess.pair("seek0004");
			waitUntil([&] { return !watch.started.isEmpty(); });
			Assert::AreEqual(QString("seek0004"), watch.started.join(","));
		}

		// Accepting a challenge meanwhile closes the seek first: one game at a time.
		TEST_METHOD(AcceptChallenge_CancelsTheSeek)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			client.acceptChallenge("chal0001");
			Assert::IsFalse(client.seeking());
			waitUntil([&] { return lichess.dropped.contains(FakeSeekLichess::Seek) && !watch.started.isEmpty(); });

			Assert::IsTrue(lichess.dropped.contains(FakeSeekLichess::Seek));
			Assert::AreEqual(QString("chal0001"), watch.started.join(","));
			Assert::IsTrue(watch.failed.isEmpty());
		}

		// Another profile, or signed out: the seek was the other account's.
		TEST_METHOD(NewToken_CancelsTheSeek)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			seek(client, lichess);
			client.setToken("another-token");
			Assert::IsFalse(client.seeking());
			Assert::AreEqual(2, watch.changes);
			waitUntil([&] { return lichess.dropped.contains(FakeSeekLichess::Seek); });

			Assert::IsTrue(lichess.dropped.contains(FakeSeekLichess::Seek));
			Assert::IsTrue(watch.failed.isEmpty());
		}

		// Closing the app closes the seek's request.
		TEST_METHOD(Destructor_ClosesTheSeek)
		{
			FakeSeekLichess lichess;
			{
				LichessClient client;
				setUp(client, lichess);
				seek(client, lichess);
			}
			waitUntil([&] { return lichess.dropped.contains(FakeSeekLichess::Seek); });

			Assert::IsTrue(lichess.dropped.contains(FakeSeekLichess::Seek));
		}

		// Correspondence: days, no clock; Lichess answers at once and keeps it. No waiting state.
		TEST_METHOD(Correspondence_SentAndLeftOnLichess)
		{
			FakeSeekLichess lichess;
			LichessClient client;
			setUp(client, lichess);
			Watch watch(client);

			client.startSeek(0, 0, 3, true, "standard");
			Assert::IsFalse(client.seeking());
			waitUntil([&] { return !watch.sent.isEmpty(); });

			const QUrlQuery body = seekBody(lichess);
			Assert::AreEqual(QString("3"), body.queryItemValue("days"));
			Assert::IsFalse(body.hasQueryItem("time"));
			Assert::IsFalse(body.hasQueryItem("increment"));
			Assert::IsTrue(watch.sent[0].contains("Only lichess.org can cancel it"));
			Assert::AreEqual(0, watch.changes);
			Assert::IsTrue(watch.failed.isEmpty());
		}

		TEST_METHOD(NoClockNoDays_Refused)
		{
			LichessClient client;
			client.setToken("token");
			Watch watch(client);

			client.startSeek(0, 0, 0, true, "standard");

			Assert::AreEqual(1, int(watch.failed.size()));
			Assert::IsFalse(client.seeking());
			client.setToken({});
		}

		// Screenshot mode: the waiting state for the scenes, nothing sent.
		TEST_METHOD(Offline_StateOnly)
		{
			LichessClient client;
			client.setOffline(true);
			Watch watch(client);

			client.startSeek(600, 0, 0, true, "standard");

			Assert::IsTrue(client.seeking());
			Assert::IsTrue(watch.failed.isEmpty());
			client.cancelSeek();
			Assert::IsFalse(client.seeking());
		}
	};
}
