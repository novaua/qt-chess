#include <QJsonDocument>
#include <QJsonObject>
#include <CppUnitTest.h>
#include "LichessClient.h"
#include "TestSupport.h"
#include "FakeLichess.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// BL-10 step 2: the account's event stream and the challenges to the player.
namespace LichessTests
{
	// A "challenge" event as Lichess sends it on /api/stream/event.
	static QByteArray challengeEvent(const char* id, const char* direction = "in", const char* extra = "",
	                                 const char* compat = R"("compat":{"bot":false,"board":true},)")
	{
		return QByteArray(R"({"type":"challenge",)") + compat + R"("challenge":{"id":")" + id
			+ R"(","url":"https://lichess.org/)" + id + R"(","status":"created",)"
			+ R"("challenger":{"id":"friend","name":"Friend","rating":1712,"provisional":true},)"
			+ R"("destUser":{"id":"me","name":"Me","rating":1500},)"
			+ R"("variant":{"key":"standard"},"rated":true,"speed":"rapid",)"
			+ R"("timeControl":{"type":"clock","limit":600,"increment":5,"show":"10+5"},)"
			+ R"("color":"white","direction":")" + direction + "\"" + extra + "}}";
	}

	static QByteArray simpleEvent(const char* type, const char* id)
	{
		return QByteArray(R"({"type":")") + type + R"(","challenge":{"id":")" + id + R"("}})";
	}

	static QJsonObject json(const QByteArray& text) { return QJsonDocument::fromJson(text).object(); }

	static QStringList ids(const LichessClient& client)
	{
		QStringList list;
		for (const auto& row : client.incomingChallenges())
			list << row.toMap().value("id").toString();
		return list;
	}

	static void areEqual(const QStringList& expected, const QStringList& actual)
	{
		Assert::AreEqual(expected.join(" | ").toStdWString(), actual.join(" | ").toStdWString());
	}

	TEST_CLASS(LichessChallengeTests)
	{
	public:
		TEST_METHOD(ParseChallenge_RealTime)
		{
			const QVariantMap row = LichessClient::parseChallenge(json(challengeEvent("c1")));
			Assert::AreEqual(QString("c1"), row.value("id").toString());
			Assert::AreEqual(QString("Friend"), row.value("challengerName").toString());
			Assert::AreEqual(1712, row.value("challengerRating").toInt());
			Assert::IsTrue(row.value("challengerProvisional").toBool());
			Assert::AreEqual(QString("10+5"), row.value("timeControlText").toString());
			Assert::AreEqual(QString("rapid"), row.value("speed").toString());
			Assert::IsTrue(row.value("rated").toBool());
			Assert::AreEqual(QString("black"), row.value("ourSide").toString(), L"the challenger plays White");
			Assert::IsTrue(row.value("canPlayHere").toBool());
			Assert::AreEqual(QString("https://lichess.org/c1"), row.value("url").toString());
		}

		TEST_METHOD(ParseChallenge_TimeControlsAndSides)
		{
			auto parse = [](const char* timeControl, const char* color) {
				return LichessClient::parseChallenge(json(QByteArray(R"({"type":"challenge","challenge":{"id":"c2",)")
					+ R"("timeControl":)" + timeControl + R"(,"color":")" + color + R"("}})"));
			};
			Assert::AreEqual(QString("3 days per move"), parse(R"({"type":"correspondence","daysPerTurn":3})", "black").value("timeControlText").toString());
			Assert::AreEqual(QString("1 day per move"), parse(R"({"type":"correspondence","daysPerTurn":1})", "black").value("timeControlText").toString());
			Assert::AreEqual(QString("Unlimited"), parse(R"({"type":"unlimited"})", "black").value("timeControlText").toString());
			Assert::AreEqual(QString("white"), parse(R"({"type":"unlimited"})", "black").value("ourSide").toString());
			Assert::AreEqual(QString("random"), parse(R"({"type":"unlimited"})", "random").value("ourSide").toString());
		}

		// Lichess's compat.board decides; Chess960 waits for LI-01; without compat, the speeds the
		// Board API plays.
		TEST_METHOD(ParseChallenge_WhatThisAppCanPlay)
		{
			auto canPlay = [](const QByteArray& event) { return LichessClient::parseChallenge(json(event)).value("canPlayHere").toBool(); };
			Assert::IsFalse(canPlay(challengeEvent("c3", "in", "", R"("compat":{"bot":false,"board":false},)")), L"e.g. Bullet");
			Assert::IsFalse(canPlay(R"({"type":"challenge","compat":{"board":true},"challenge":{"id":"c4","variant":{"key":"chess960"},"speed":"rapid"}})"));
			Assert::IsFalse(canPlay(R"({"type":"challenge","challenge":{"id":"c5","speed":"blitz"}})"));
			Assert::IsTrue(canPlay(R"({"type":"challenge","challenge":{"id":"c6","speed":"classical"}})"));
		}

		TEST_METHOD(EventLine_ChallengeToUs_ListedOnce_ThenCanceled)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList received;
			QObject::connect(&client, &LichessClient::challengeReceived, [&](QString id) { received << id; });

			client.handleEventStreamLine(challengeEvent("c1"));
			client.handleEventStreamLine(challengeEvent("c1")); // resent, e.g. after a reconnect
			areEqual({ "c1" }, ids(client));
			areEqual({ "c1" }, received);

			client.handleEventStreamLine(simpleEvent("challengeCanceled", "c1"));
			Assert::IsTrue(client.incomingChallenges().isEmpty());
		}

		// The stream shows our own challenges too.
		TEST_METHOD(EventLine_OurOwnChallenges_NotListed)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleEventStreamLine(challengeEvent("c1", "out"));
			client.handleEventStreamLine(R"({"type":"challenge","challenge":{"id":"c2","challenger":{"id":"me"},"destUser":{"id":"friend"}}})");
			client.handleEventStreamLine(R"({"type":"challenge","challenge":{"id":"c3","challenger":{"id":"friend"},"destUser":{"id":"someoneelse"}}})");
			Assert::IsTrue(client.incomingChallenges().isEmpty());

			// Without "direction" (older events): to us when we are its destUser.
			client.handleEventStreamLine(R"({"type":"challenge","challenge":{"id":"c4","challenger":{"id":"friend"},"destUser":{"id":"me"}}})");
			areEqual({ "c4" }, ids(client));
		}

		// Accepted (here or in a browser): its game starts with the same id.
		TEST_METHOD(EventLine_GameStartOfAChallenge_RemovesIt)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleEventStreamLine(challengeEvent("c1"));
			client.handleEventStreamLine(challengeEvent("c2"));
			client.handleEventStreamLine(R"({"type":"gameStart","game":{"gameId":"c1","fullId":"c1abcd"}})");
			client.handleEventStreamLine(simpleEvent("challengeDeclined", "c2"));
			Assert::IsTrue(client.incomingChallenges().isEmpty());
		}

		TEST_METHOD(TokenChange_ClearsTheChallenges)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleEventStreamLine(challengeEvent("c1"));
			client.setToken("another-account-token");
			Assert::IsTrue(client.incomingChallenges().isEmpty());
			Assert::IsFalse(client.eventStreamOpen(), L"setToken doesn't open it: main.cpp does");
			client.setToken({});
		}
	};

	// Against FakeLichess: real sockets, no Lichess.
	TEST_CLASS(LichessEventStreamTests)
	{
		static const inline QString Events = QStringLiteral("GET /api/stream/event");

		static void setUp(LichessClient& client, const FakeLichess& lichess)
		{
			client.setUsername("me");
			client.setBaseUrl(lichess.url());
			client.setStreamTiming(5000, 100, 5000, 300);
			client.setToken("token");
		}

	public:
		TEST_METHOD(ChallengeArrives_ThenIsCanceled)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, challengeEvent("c1"));
				QTimer::singleShot(200, socket, [socket]() { FakeLichess::line(socket, simpleEvent("challengeCanceled", "c1")); });
			};
			LichessClient client;
			setUp(client, lichess);
			QStringList received;
			QObject::connect(&client, &LichessClient::challengeReceived, [&](QString id) { received << id; });

			client.startEventStream();
			waitUntil([&] { return !received.isEmpty(); });
			areEqual({ "c1" }, ids(client));
			waitUntil([&] { return client.incomingChallenges().isEmpty(); });

			Assert::IsTrue(client.incomingChallenges().isEmpty());
			Assert::AreEqual(1, lichess.count(Events));
		}

		// After a reconnect Lichess sends all current challenges again: one is new to nobody, and one
		// canceled while we were away (its challengeCanceled missed) is gone.
		TEST_METHOD(Reconnect_ResentOnce_GoneOnesDropped)
		{
			FakeLichess lichess;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, challengeEvent("c1"));
				if (lichess.count(Events) == 1) {
					FakeLichess::line(socket, challengeEvent("c2"));
					QTimer::singleShot(100, socket, [socket]() { socket->disconnectFromHost(); });
				}
			};
			LichessClient client;
			setUp(client, lichess);
			QStringList received;
			QObject::connect(&client, &LichessClient::challengeReceived, [&](QString id) { received << id; });

			client.startEventStream();
			waitUntil([&] { return lichess.count(Events) == 2 && ids(client) == QStringList{ "c1" }; });

			areEqual({ "c1" }, ids(client));
			areEqual({ "c1", "c2" }, received);
		}

		// One event stream per token (Lichess closes the older one): waiting for our challenge's game
		// uses the open stream, and leaving a game doesn't close it.
		TEST_METHOD(WaitForGameStart_SharesTheStream)
		{
			FakeLichess lichess;
			QTcpSocket* events = nullptr;
			lichess.onRequest = [&](QTcpSocket* socket, const QString&, const QString& path) {
				FakeLichess::startStream(socket);
				if (path == "/api/stream/event")
					events = socket;
				else
					FakeLichess::line(socket, R"({"type":"gameFull","id":"g9","white":{"id":"me"},"black":{"id":"friend"},"state":{"moves":"","status":"started"}})");
			};
			LichessClient client;
			setUp(client, lichess);
			QStringList started;
			QObject::connect(&client, &LichessClient::gameStarted, [&](QString id, bool, QString, QString) { started << id; });

			client.startEventStream();
			waitUntil([&] { return events != nullptr; });
			client.waitForGameStart("g9", false);
			FakeLichess::line(events, R"({"type":"gameStart","game":{"gameId":"g9","fullId":"g9abcd"}})");
			waitUntil([&] { return !started.isEmpty(); });

			areEqual({ "g9" }, started);
			Assert::AreEqual(1, lichess.count(Events));
			client.stopStream();
			Assert::IsTrue(client.eventStreamOpen(), L"the account's stream, not the game's");
		}

		TEST_METHOD(Decline_SendsTheReason)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString& method, const QString&) {
				if (method == "POST") {
					FakeLichess::json(socket, R"({"ok":true})");
					return;
				}
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, challengeEvent("c1"));
			};
			LichessClient client;
			setUp(client, lichess);
			QStringList errors;
			QObject::connect(&client, &LichessClient::networkError, [&](QString message) { errors << message; });

			client.startEventStream();
			waitUntil([&] { return !client.incomingChallenges().isEmpty(); });
			client.declineChallenge("c1", "tooFast");
			Assert::IsTrue(client.incomingChallenges().isEmpty(), L"gone at once");
			waitUntil([&] { return lichess.requests.contains("POST /api/challenge/c1/decline"); });
			waitUntil([] { return false; }, 200);

			const int post = int(lichess.requests.indexOf("POST /api/challenge/c1/decline"));
			Assert::AreEqual(QString("reason=tooFast"), lichess.bodies.value(post));
			Assert::IsTrue(errors.isEmpty());
		}

		// Games started or ended elsewhere: Your games (LI-04) refreshed once for a burst of events.
		TEST_METHOD(GamesElsewhere_RefreshYourGamesOnce)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString& path) {
				if (path.startsWith("/api/account/playing")) {
					FakeLichess::json(socket, R"({"nowPlaying":[]})");
					return;
				}
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, R"({"type":"gameStart","game":{"gameId":"g1"}})");
				FakeLichess::line(socket, R"({"type":"gameStart","game":{"gameId":"g2"}})");
				FakeLichess::line(socket, R"({"type":"gameFinish","game":{"gameId":"g3"}})");
			};
			LichessClient client;
			setUp(client, lichess);

			client.startEventStream();
			waitUntil([] { return false; }, 1200);

			Assert::AreEqual(1, lichess.count("GET /api/account/playing?nb=50"));
		}

		// Another profile's account: its stream goes; main.cpp opens one for the new token.
		TEST_METHOD(TokenChange_ClosesTheStream)
		{
			FakeLichess lichess;
			lichess.onRequest = [](QTcpSocket* socket, const QString&, const QString&) {
				FakeLichess::startStream(socket);
				FakeLichess::line(socket, challengeEvent("c1"));
			};
			LichessClient client;
			setUp(client, lichess);

			client.startEventStream();
			waitUntil([&] { return !client.incomingChallenges().isEmpty(); });
			client.setToken("another-token");
			Assert::IsFalse(client.eventStreamOpen());
			Assert::IsTrue(client.incomingChallenges().isEmpty());
			client.startEventStream();
			waitUntil([&] { return lichess.count(Events) == 2; });
			Assert::AreEqual(2, lichess.count(Events));
		}
	};
}
