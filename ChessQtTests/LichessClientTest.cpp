#include <QObject>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <CppUnitTest.h>
#include "LichessClient.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ---------------------------------------------------------------------------
// Unit tests — pure static methods, no network, no event loop needed
// ---------------------------------------------------------------------------
namespace LichessTests
{
	TEST_CLASS(LichessStaticTests)
	{
		LichessClient _client;

	public:

		// gameIdFromUrl —————————————————————————————————————————————————————

		TEST_METHOD(GameIdFromUrl_FullUrl_ReturnsId)
		{
			auto id = _client.gameIdFromUrl("https://lichess.org/AbCdEfGh");
			Assert::AreEqual(QString("AbCdEfGh"), id);
		}

		TEST_METHOD(GameIdFromUrl_FullUrlWithColorSuffix_ReturnsId)
		{
			auto id = _client.gameIdFromUrl("https://lichess.org/AbCdEfGh/white");
			Assert::AreEqual(QString("AbCdEfGh"), id);
		}

		// The links Lichess gives out for an open challenge (bug: "…/game/<id>" was rejected).
		TEST_METHOD(GameIdFromUrl_ChallengeLink_ReturnsId)
		{
			Assert::AreEqual(QString("aEPqtBMc"), _client.gameIdFromUrl("https://lichess.org/game/aEPqtBMc"));
			Assert::AreEqual(QString("OHp33WmD"), _client.gameIdFromUrl("https://lichess.org/game/OHp33WmD?color=white"));
		}

		TEST_METHOD(ColorFromUrl_PlayerLinks_Test)
		{
			Assert::AreEqual(QString("black"), LichessClient::colorFromUrl("https://lichess.org/game/OHp33WmD?color=black"));
			Assert::AreEqual(QString("white"), LichessClient::colorFromUrl("https://lichess.org/game/OHp33WmD?color=white"));
			Assert::AreEqual(QString("black"), LichessClient::colorFromUrl("https://lichess.org/AbCdEfGh/black"));
			Assert::IsTrue(LichessClient::colorFromUrl("https://lichess.org/game/OHp33WmD").isEmpty(), L"random: no side");
			Assert::IsTrue(LichessClient::colorFromUrl("AbCdEfGh").isEmpty());
			Assert::IsTrue(LichessClient::colorFromUrl("https://lichess.org/whitexyz").isEmpty(), L"an id, not a side");
		}

		TEST_METHOD(GameIdFromUrl_BareId_ReturnsSame)
		{
			auto id = _client.gameIdFromUrl("AbCdEfGh");
			Assert::AreEqual(QString("AbCdEfGh"), id);
		}

		TEST_METHOD(GameIdFromUrl_BareIdWithSpaces_ReturnsTrimmed)
		{
			auto id = _client.gameIdFromUrl("  AbCdEfGh  ");
			Assert::AreEqual(QString("AbCdEfGh"), id);
		}

		TEST_METHOD(GameIdFromUrl_TooShort_ReturnsEmpty)
		{
			auto id = _client.gameIdFromUrl("AbCdEfG");  // 7 chars
			Assert::IsTrue(id.isEmpty(), L"7-char string should not match");
		}

		TEST_METHOD(GameIdFromUrl_TooLong_ReturnsEmpty)
		{
			auto id = _client.gameIdFromUrl("AbCdEfGhI");  // 9 chars
			Assert::IsTrue(id.isEmpty(), L"9-char bare string should not match");
		}

		TEST_METHOD(GameIdFromUrl_NonAlphanumeric_ReturnsEmpty)
		{
			auto id = _client.gameIdFromUrl("Ab!dEfGh");
			Assert::IsTrue(id.isEmpty(), L"Special chars should not match");
		}

		TEST_METHOD(GameIdFromUrl_Empty_ReturnsEmpty)
		{
			auto id = _client.gameIdFromUrl("");
			Assert::IsTrue(id.isEmpty());
		}

		// encryptToken / decryptToken ————————————————————————————————————————

		TEST_METHOD(EncryptDecryptToken_TypicalToken_RoundTrips)
		{
			const QString token = "lip_AbCdEf1234567890";
			const QString cipher = _client.encryptToken(token);
			Assert::IsFalse(cipher.isEmpty(), L"Encrypted token must not be empty");

			const QString recovered = _client.decryptToken(cipher);
			Assert::AreEqual(token, recovered);
		}

		TEST_METHOD(DecryptToken_EmptyInput_ReturnsEmpty)
		{
			const QString result = _client.decryptToken("");
			Assert::IsTrue(result.isEmpty());
		}

		TEST_METHOD(EncryptDecryptToken_SpecialChars_RoundTrips)
		{
			const QString token = QString::fromUtf8("tok\xC3\xA9n with spaces & symbols!");
			const QString cipher = _client.encryptToken(token);
			const QString recovered = _client.decryptToken(cipher);
			Assert::AreEqual(token, recovered);
		}
		// resolveIsWhite ————————————————————————————————————————————————————

		// myColor present — takes priority over everything else
		TEST_METHOD(ResolveIsWhite_MyColorWhite_ReturnsTrue)
		{
			Assert::IsTrue(LichessClient::resolveIsWhite("white", "opponent", "me"));
		}

		TEST_METHOD(ResolveIsWhite_MyColorBlack_ReturnsFalse)
		{
			Assert::IsFalse(LichessClient::resolveIsWhite("black", "opponent", "me"));
		}

		// myColor absent — fall back to username vs whiteId
		TEST_METHOD(ResolveIsWhite_NoMyColor_UsernameMatchesWhite_ReturnsTrue)
		{
			Assert::IsTrue(LichessClient::resolveIsWhite("", "vgerman256", "vgerman256"));
		}

		TEST_METHOD(ResolveIsWhite_NoMyColor_UsernameIsBlack_ReturnsFalse)
		{
			Assert::IsFalse(LichessClient::resolveIsWhite("", "opponent", "vgerman256"));
		}

		// myColor absent AND no username — default white
		TEST_METHOD(ResolveIsWhite_NoMyColor_NoUsername_DefaultsToWhite)
		{
			Assert::IsTrue(LichessClient::resolveIsWhite("", "anyone", ""));
		}

		// myColor takes priority even when username would disagree
		TEST_METHOD(ResolveIsWhite_MyColorBlack_IgnoresUsername)
		{
			// Username matches white.id but myColor says black — myColor wins
			Assert::IsFalse(LichessClient::resolveIsWhite("black", "me", "me"));
		}

		// parseRematchInfo ——————————————————————————————————————————————————

		static QJsonObject Json(const char* text)
		{
			return QJsonDocument::fromJson(QByteArray(text)).object();
		}

		TEST_METHOD(ParseRematchInfo_RegisteredOpponent_IdNameClockVariant)
		{
			auto info = LichessClient::parseRematchInfo(Json(R"({
				"type": "gameFull", "id": "AbCdEfGh",
				"variant": { "key": "chess960", "name": "Chess960" },
				"clock": { "initial": 300000, "increment": 3000 },
				"white": { "id": "me", "name": "Me" },
				"black": { "id": "friend", "name": "Friend" }
			})"), "me");

			Assert::IsTrue(info.valid);
			Assert::AreEqual(QString("AbCdEfGh"), info.gameId);
			Assert::IsTrue(info.playedAsWhite);
			Assert::AreEqual(QString("friend"), info.opponentId);
			Assert::AreEqual(QString("Friend"), info.opponentName);
			Assert::AreEqual(300, info.clockLimitSec);
			Assert::AreEqual(3, info.clockIncrementSec);
			Assert::AreEqual(0, info.days);
			Assert::AreEqual(QString("chess960"), info.variant);
		}

		TEST_METHOD(ParseRematchInfo_AnonymousOpponent_EmptyId_Test)
		{
			auto info = LichessClient::parseRematchInfo(Json(R"({
				"variant": { "key": "standard" },
				"clock": { "initial": 600000, "increment": 0 },
				"white": {},
				"black": { "id": "me", "name": "Me" }
			})"), "me");

			Assert::IsTrue(info.valid);
			Assert::IsFalse(info.playedAsWhite);
			Assert::IsTrue(info.opponentId.isEmpty(), L"Anonymous player has no id");
			Assert::AreEqual(QString("Anonymous"), info.opponentName);
		}

		TEST_METHOD(ParseRematchInfo_Correspondence_DaysNoClock_Test)
		{
			auto info = LichessClient::parseRematchInfo(Json(R"({
				"daysPerTurn": 3,
				"white": { "id": "friend", "name": "Friend" },
				"black": { "id": "me", "name": "Me" }
			})"), "me");

			Assert::IsTrue(info.valid);
			Assert::AreEqual(0, info.clockLimitSec);
			Assert::AreEqual(3, info.days);
			Assert::AreEqual(QString("standard"), info.variant, L"Missing variant defaults to standard");
		}

		TEST_METHOD(ParseRematchInfo_LichessAi_NotRematchable_Test)
		{
			auto info = LichessClient::parseRematchInfo(Json(R"({
				"white": { "id": "me", "name": "Me" },
				"black": { "aiLevel": 3 }
			})"), "me");

			Assert::IsFalse(info.valid);
		}

		// isRematchOffer ————————————————————————————————————————————————————

		TEST_METHOD(IsRematchOffer_OpponentsOfferAfterOurGame_True)
		{
			Assert::IsTrue(LichessClient::isRematchOffer(Json(R"({
				"id": "NeXtGaMe", "rematchOf": "AbCdEfGh",
				"challenger": { "id": "friend" }, "destUser": { "id": "me" }
			})"), "AbCdEfGh", "me"));
		}

		TEST_METHOD(IsRematchOffer_OfAnotherGame_False)
		{
			Assert::IsFalse(LichessClient::isRematchOffer(Json(R"({
				"id": "NeXtGaMe", "rematchOf": "OtHeRgAm", "challenger": { "id": "friend" }
			})"), "AbCdEfGh", "me"));
		}

		TEST_METHOD(IsRematchOffer_PlainChallenge_False)
		{
			Assert::IsFalse(LichessClient::isRematchOffer(Json(R"({
				"id": "NeXtGaMe", "challenger": { "id": "friend" }
			})"), "AbCdEfGh", "me"));
		}

		TEST_METHOD(IsRematchOffer_OurOwnOffer_False)
		{
			Assert::IsFalse(LichessClient::isRematchOffer(Json(R"({
				"id": "NeXtGaMe", "rematchOf": "AbCdEfGh", "challenger": { "id": "me" }
			})"), "AbCdEfGh", "me"));
		}

		TEST_METHOD(IsRematchOffer_NoLastGame_False)
		{
			Assert::IsFalse(LichessClient::isRematchOffer(Json(R"({
				"id": "NeXtGaMe", "rematchOf": "", "challenger": { "id": "friend" }
			})"), "", "me"));
		}

		// parseRatings ———————————————————————————————————————————————————————

		TEST_METHOD(ParseRatings_PoolsWithARating_ProvisionalKept)
		{
			auto ratings = LichessClient::parseRatings(Json(R"({
				"username": "Me",
				"perfs": {
					"blitz":  { "games": 214, "rating": 1612, "rd": 60, "prog": 12 },
					"rapid":  { "games": 3, "rating": 1500, "rd": 300, "prov": true },
					"puzzle": { "games": 10, "rating": 1900 },
					"storm":  { "runs": 4, "score": 20 }
				}
			})"));

			Assert::AreEqual(2, (int)ratings.size(), L"blitz and rapid; puzzle and storm aren't games");
			const auto blitz = ratings.value("blitz").toMap();
			Assert::AreEqual(1612, blitz.value("rating").toInt());
			Assert::AreEqual(214, blitz.value("games").toInt());
			Assert::IsFalse(blitz.value("prov").toBool());
			Assert::IsTrue(ratings.value("rapid").toMap().value("prov").toBool());
		}

		TEST_METHOD(ParseRatings_NoPerfs_Empty)
		{
			Assert::IsTrue(LichessClient::parseRatings(Json(R"({ "username": "Me" })")).isEmpty());
		}

		// parseOngoingGames (LI-04) ———————————————————————————————————————————

		TEST_METHOD(ParseOngoingGames_PlayerAnonymousAiAndVariants_Test)
		{
			auto games = LichessClient::parseOngoingGames(Json(R"({
				"nbMyTurn": 1,
				"nowPlaying": [
					{ "gameId": "rapid001", "fullId": "rapid001abcd", "color": "white", "fen": "x", "lastMove": "e7e5",
					  "isMyTurn": true, "secondsLeft": 540, "hasMoved": true, "rated": true,
					  "opponent": { "id": "friend", "username": "Friend", "rating": 1650 },
					  "speed": "rapid", "source": "friend", "variant": { "key": "standard", "name": "Standard" } },
					{ "gameId": "corr0001", "color": "black", "isMyTurn": false, "secondsLeft": 190000,
					  "opponent": { "id": null, "username": "Anonymous" },
					  "speed": "correspondence", "source": "friend", "variant": { "key": "standard" } },
					{ "gameId": "aigame01", "color": "white", "isMyTurn": true,
					  "opponent": { "id": null, "username": "AI", "ai": 3 },
					  "speed": "correspondence", "source": "ai", "variant": { "key": "standard" } },
					{ "gameId": "c960game", "color": "white", "isMyTurn": true, "secondsLeft": 600,
					  "opponent": { "id": "x", "username": "X" },
					  "speed": "rapid", "source": "friend", "variant": { "key": "chess960" } }
				]
			})"));

			Assert::AreEqual(4, (int)games.size());
			const auto rapid = games[0].toMap();
			Assert::AreEqual(QString("rapid001"), rapid.value("gameId").toString(), L"Lichess's order kept");
			Assert::AreEqual(QString("white"), rapid.value("color").toString());
			Assert::AreEqual(QString("Friend"), rapid.value("opponentName").toString());
			Assert::AreEqual(1650, rapid.value("opponentRating").toInt());
			Assert::IsTrue(rapid.value("isMyTurn").toBool());
			Assert::AreEqual(QString("9 min"), rapid.value("timeLeftText").toString());
			Assert::IsTrue(rapid.value("canPlayHere").toBool());
			Assert::AreEqual(QString("e7e5"), rapid.value("lastMove").toString());

			const auto corr = games[1].toMap();
			Assert::AreEqual(QString("Anonymous"), corr.value("opponentName").toString());
			Assert::AreEqual(QString("2 days"), corr.value("timeLeftText").toString());
			Assert::IsTrue(corr.value("canPlayHere").toBool());

			const auto ai = games[2].toMap();
			Assert::AreEqual(QString("Lichess AI level 3"), ai.value("opponentName").toString());
			Assert::IsTrue(ai.value("opponentIsAi").toBool());
			Assert::AreEqual(-1, ai.value("secondsLeft").toInt(), L"no clock");
			Assert::IsTrue(ai.value("timeLeftText").toString().isEmpty());

			Assert::IsFalse(games[3].toMap().value("canPlayHere").toBool(), L"Chess960 isn't playable here yet (LI-01)");
		}

		// The Board API: Rapid, Classical, Correspondence; Blitz only in direct challenges and vs AI.
		TEST_METHOD(ParseOngoingGames_SpeedsTheBoardApiPlays_Test)
		{
			auto games = LichessClient::parseOngoingGames(Json(R"({ "nowPlaying": [
				{ "gameId": "blitzFrn", "speed": "blitz",  "source": "friend", "variant": { "key": "standard" } },
				{ "gameId": "blitzLob", "speed": "blitz",  "source": "lobby",  "variant": { "key": "standard" } },
				{ "gameId": "bulletLo", "speed": "bullet", "source": "lobby",  "variant": { "key": "standard" } },
				{ "gameId": "classicL", "speed": "classical", "source": "lobby", "variant": { "key": "standard" } },
				{ "gameId": "blitzApi", "speed": "blitz",  "source": "api",    "variant": { "key": "standard" } }
			] })"));

			Assert::AreEqual(5, (int)games.size());
			Assert::IsTrue(games[0].toMap().value("canPlayHere").toBool());
			Assert::IsFalse(games[1].toMap().value("canPlayHere").toBool());
			Assert::IsFalse(games[2].toMap().value("canPlayHere").toBool());
			Assert::IsTrue(games[3].toMap().value("canPlayHere").toBool());
			Assert::IsTrue(games[4].toMap().value("canPlayHere").toBool(), L"an API challenge or bulk pairing");
		}

		TEST_METHOD(ParseOngoingGames_NoGames_Empty)
		{
			Assert::IsTrue(LichessClient::parseOngoingGames(Json(R"({ "nbMyTurn": 0, "nowPlaying": [] })")).isEmpty());
			Assert::IsTrue(LichessClient::parseOngoingGames(Json(R"({})")).isEmpty());
		}

		// Another profile's account (or none): the games shown belong to the old one.
		TEST_METHOD(OngoingGames_TokenChange_ClearsTheList_Test)
		{
			LichessClient client;
			client.setOngoingGames(LichessClient::parseOngoingGames(Json(R"({ "nowPlaying": [
				{ "gameId": "rapid001", "speed": "rapid", "variant": { "key": "standard" } } ] })")));
			Assert::AreEqual(1, (int)client.ongoingGames().size());

			client.setToken("another-account-token");

			Assert::IsTrue(client.ongoingGames().isEmpty());
			Assert::IsFalse(client.ongoingGamesLoading());
			client.setToken({}); // no network call was made: nothing to clean up
		}

		// It goes into the URL of the challenge.
		TEST_METHOD(IsUsername_Test)
		{
			Assert::IsTrue(LichessClient::isUsername("DrNykterstein"));
			Assert::IsTrue(LichessClient::isUsername("a_b-3"));
			Assert::IsTrue(LichessClient::isUsername("ab"));
			Assert::IsTrue(LichessClient::isUsername(QString(30, 'x')));
			Assert::IsFalse(LichessClient::isUsername(""));
			Assert::IsFalse(LichessClient::isUsername("a"));
			Assert::IsFalse(LichessClient::isUsername(QString(31, 'x')));
			Assert::IsFalse(LichessClient::isUsername("two words"));
			Assert::IsFalse(LichessClient::isUsername("../account"));
			Assert::IsFalse(LichessClient::isUsername("name?x=1"));
			Assert::IsFalse(LichessClient::isUsername("name\n"));
			Assert::IsFalse(LichessClient::isUsername("open"), L"/api/challenge/open is another call");
			Assert::IsFalse(LichessClient::isUsername("ai"), L"/api/challenge/ai is another call");
		}

		TEST_METHOD(NeedsKeepAlive_OnlyWithAClock_Test)
		{
			RematchInfo game;
			Assert::IsFalse(LichessClient::needsKeepAlive(game), L"unlimited");
			game.days = 3;
			Assert::IsFalse(LichessClient::needsKeepAlive(game), L"correspondence");
			game = {};
			game.clockLimitSec = 300;
			Assert::IsTrue(LichessClient::needsKeepAlive(game));
			game = {};
			game.clockIncrementSec = 1; // 0+1
			Assert::IsTrue(LichessClient::needsKeepAlive(game));
		}

		// A bad name never reaches Lichess: the error comes at once.
		TEST_METHOD(ChallengePlayer_BadName_ErrorWithoutARequest_Test)
		{
			LichessClient client;
			QStringList errors;
			QObject::connect(&client, &LichessClient::networkError, [&](QString message) { errors << message; });
			client.challengePlayer("../account", 300, 3, 0, "random");
			Assert::AreEqual(1, (int)errors.size());
		}

		TEST_METHOD(TimeLeftText_Units_Test)
		{
			Assert::AreEqual(QString("45 s"),   LichessClient::timeLeftText(45));
			Assert::AreEqual(QString("119 s"),  LichessClient::timeLeftText(119), L"not \"1 min\"");
			Assert::AreEqual(QString("3 min"),  LichessClient::timeLeftText(200));
			Assert::AreEqual(QString("119 min"), LichessClient::timeLeftText(2 * 3600 - 1));
			Assert::AreEqual(QString("5 h"),    LichessClient::timeLeftText(5 * 3600 + 100));
			Assert::AreEqual(QString("47 h"),   LichessClient::timeLeftText(47 * 3600 + 59), L"not \"1 day\"");
			Assert::AreEqual(QString("2 days"), LichessClient::timeLeftText(2 * 86400));
			Assert::AreEqual(QString("14 days"), LichessClient::timeLeftText(14 * 86400));
			Assert::IsTrue(LichessClient::timeLeftText(-1).isEmpty(), L"no clock");
		}

		// handleGameStreamLine ————————————————————————————————————————————————

		TEST_METHOD(GameStream_GameStateWithMate_MoveBeforeEnd_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			QObject::connect(&client, &LichessClient::opponentMoveReceived, [&](QString moves) { events << "move " + moves; });
			QObject::connect(&client, &LichessClient::gameEnded, [&](QString status, QString winner) { events << "end " + status + " " + winner; });

			Assert::IsTrue(client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"f2f3 e7e5 g2g4","status":"started"}})"));
			events.clear(); // the resync of the moves before we joined
			// The opponent's mate comes in the same state as the end of the game.
			Assert::IsFalse(client.handleGameStreamLine(R"({"type":"gameState","moves":"f2f3 e7e5 g2g4 d8h4",
				"status":"mate","winner":"black"})"), L"the stream is over after the end");

			Assert::AreEqual(2, (int)events.size());
			Assert::AreEqual(QString("move d8h4"), events[0], L"the mating move first");
			Assert::AreEqual(QString("end mate black"), events[1]);
		}

		// The clock (LI-02): what the stream sends, in the order the connector needs it.
		struct ClockEvents {
			QStringList events;
			explicit ClockEvents(LichessClient& client) {
				QObject::connect(&client, &LichessClient::gameStarted, [this] { events << "started"; });
				QObject::connect(&client, &LichessClient::clockStarted, [this](qint64 limit, qint64 inc, bool days) {
					events << QString("clock %1 %2 %3").arg(limit).arg(inc).arg(days ? "days" : "real");
				});
				QObject::connect(&client, &LichessClient::opponentMoveReceived, [this](QString moves) { events << "move " + moves; });
				QObject::connect(&client, &LichessClient::clockUpdated, [this](qint64 white, qint64 black, bool whiteToMove, bool running) {
					events << QString("times %1 %2 %3 %4").arg(white).arg(black).arg(whiteToMove ? "w" : "b").arg(running ? "running" : "stopped");
				});
				QObject::connect(&client, &LichessClient::gameEnded, [this](QString status, QString) { events << "end " + status; });
			}
		};

		static void areEqual(const QStringList& expected, const QStringList& actual)
		{
			Assert::AreEqual(expected.join(" | ").toStdWString(), actual.join(" | ").toStdWString());
		}

		TEST_METHOD(GameStream_Clock_StartedBeforeTheMoves_TimesAfter_Test)
		{
			LichessClient client;
			client.setUsername("me");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":5000},
				"state":{"moves":"e2e4 e7e5 g1f3","wtime":590000,"btime":575000,"status":"started"}})");
			areEqual({ "started", "clock 600000 5000 real", "move e2e4 e7e5 g1f3", "times 590000 575000 b running" }, clock.events);
		}

		// Lichess starts the clocks once both sides have moved.
		TEST_METHOD(GameStream_Clock_RunsFromThe2ndPly_Test)
		{
			LichessClient client;
			client.setUsername("me");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":0},
				"state":{"moves":"","wtime":600000,"btime":600000,"status":"started"}})");
			client.trackSentMove("e2e4");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4","wtime":600000,"btime":600000,"status":"started"})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","wtime":600000,"btime":600000,"status":"started"})");
			areEqual({ "started", "clock 600000 0 real", "times 600000 600000 w stopped", "times 600000 600000 b stopped",
			           "move e7e5", "times 600000 600000 w running" }, clock.events);
		}

		// Our move is on our board and switched our clock: a state from before it must not switch back.
		TEST_METHOD(GameStream_Clock_StateWithoutOurSentMove_Ignored_Test)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":0},
				"state":{"moves":"e2e4 e7e5","wtime":600000,"btime":600000,"status":"started"}})");
			ClockEvents clock(client);
			client.trackSentMove("g1f3");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","wtime":599000,"btime":600000,"status":"started","bdraw":true})");
			Assert::IsTrue(clock.events.isEmpty());
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3","wtime":598000,"btime":600000,"status":"started"})");
			areEqual({ "times 598000 600000 b running" }, clock.events);
		}

		// An accepted takeback: the list first, then the times of the position it leaves.
		TEST_METHOD(GameStream_Clock_AfterATakeback_Test)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":0},
				"state":{"moves":"e2e4 e7e5 g1f3","wtime":600000,"btime":600000,"status":"started"}})");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","wtime":590000,"btime":580000,"status":"started"})");
			areEqual({ "times 590000 580000 w running" }, clock.events);
		}

		// The state that ends the game: its last move, its final times, stopped, then the end.
		TEST_METHOD(GameStream_Clock_StopsAtTheEnd_Test)
		{
			LichessClient client;
			client.setUsername("me");
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":0},
				"state":{"moves":"e2e4 e7e5","wtime":120,"btime":600000,"status":"started"}})");
			ClockEvents clock(client);
			client.trackSentMove("g1f3"); // on its way when the time ran out
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","wtime":0,"btime":600000,"status":"outoftime","winner":"black"})");
			areEqual({ "times 0 600000 w stopped", "end outoftime" }, clock.events);
		}

		TEST_METHOD(GameStream_Clock_FinishedGame_Stopped_Test)
		{
			LichessClient client;
			client.setUsername("me");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"clock":{"initial":600000,"increment":0},
				"state":{"moves":"e2e4 e7e5","wtime":400000,"btime":300000,"status":"resign","winner":"black"}})");
			areEqual({ "started", "clock 600000 0 real", "move e2e4 e7e5", "times 400000 300000 w stopped", "end resign" }, clock.events);
		}

		// BotJsonView.scala: wtime and btime are the time left for the move, and it runs from the start.
		TEST_METHOD(GameStream_Clock_Correspondence_Test)
		{
			LichessClient client;
			client.setUsername("me");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},"daysPerTurn":14,
				"state":{"moves":"","wtime":1209000000,"btime":1209600000,"status":"started"}})");
			areEqual({ "started", "clock 1209600000 0 days", "times 1209000000 1209600000 w running" }, clock.events);
		}

		// No clock at all: Lichess sends the largest int, which isn't a time.
		TEST_METHOD(GameStream_NoClock_NoTimes_Test)
		{
			LichessClient client;
			client.setUsername("me");
			ClockEvents clock(client);
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4 e7e5","wtime":2147483647,"btime":2147483647,"status":"started"}})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3","wtime":2147483647,"btime":2147483647,"status":"started"})");
			areEqual({ "started", "move e2e4 e7e5", "move g1f3" }, clock.events);
		}

		TEST_METHOD(GameStream_GameFull_ResyncsAllMovesInOneSignal_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			QObject::connect(&client, &LichessClient::gameStarted, [&](QString, bool, QString, QString) { events << "start"; });
			QObject::connect(&client, &LichessClient::opponentMoveReceived, [&](QString moves) { events << "move " + moves; });

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4 e7e5 g1f3","status":"started"}})");

			Assert::AreEqual(2, (int)events.size());
			Assert::AreEqual(QString("start"), events[0], L"the board is set up before the moves");
			Assert::AreEqual(QString("move e2e4 e7e5 g1f3"), events[1]);
		}

		TEST_METHOD(GameStream_GameState_OnlyNewMoves_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			QObject::connect(&client, &LichessClient::opponentMoveReceived, [&](QString moves) { events << moves; });

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4","status":"started"}})");
			events.clear();
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started"})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started","wdraw":true})");

			Assert::AreEqual(1, (int)events.size(), L"a state without a new move emits none");
			Assert::AreEqual(QString("e7e5"), events[0]);
		}

		// LI-03: the stream's move list against ours (Lichess's, plus our moves sent but not echoed yet).
		static void recordMoveEvents(LichessClient& client, QStringList& events)
		{
			QObject::connect(&client, &LichessClient::opponentMoveReceived, [&](QString moves) { events << "move " + moves; });
			QObject::connect(&client, &LichessClient::moveListReplaced, [&](QString moves, bool) { events << "replaced " + moves; });
		}

		TEST_METHOD(GameStream_AcceptedTakeback_ReplacesTheList_ThenGrowsAgain_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			recordMoveEvents(client, events);

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4 e7e5 g1f3 b8c6","status":"started"}})");
			events.clear();
			// Our takeback request, accepted: Lichess's list loses Nc6 and Nf3.
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3 b8c6","status":"started","wtakeback":true})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started"})");
			Assert::AreEqual(1, (int)events.size(), L"only the shorter list changes anything");
			Assert::AreEqual(QString("replaced e2e4 e7e5"), events[0]);

			// The game goes on from there: our new move's echo, then the opponent's answer.
			events.clear();
			client.trackSentMove("d2d4");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 d2d4","status":"started"})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 d2d4 e5d4","status":"started"})");
			Assert::AreEqual(1, (int)events.size());
			Assert::AreEqual(QString("move e5d4"), events[0]);
		}

		TEST_METHOD(GameStream_OpponentsMoveTakenBack_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			recordMoveEvents(client, events);

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4","status":"started"}})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 f7f6","status":"started"})");
			events.clear();
			// The opponent asked to take back f6 and we accepted.
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4","status":"started"})");
			Assert::AreEqual(1, (int)events.size());
			Assert::AreEqual(QString("replaced e2e4"), events[0]);
		}

		TEST_METHOD(GameStream_SentMoveNotInTheStateYet_Waits_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			recordMoveEvents(client, events);

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4 e7e5","status":"started"}})");
			events.clear();
			client.trackSentMove("g1f3");
			// A draw offer's state before Lichess has played our move: it isn't a takeback.
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started","bdraw":true})");
			Assert::AreEqual(0, (int)events.size(), L"our sent move is still on its way");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3","status":"started"})");
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 g1f3 b8c6","status":"started"})");
			Assert::AreEqual(1, (int)events.size(), L"the echo of our move isn't the opponent's");
			Assert::AreEqual(QString("move b8c6"), events[0]);
		}

		TEST_METHOD(GameStream_ListsDisagree_ReplacesTheList_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			recordMoveEvents(client, events);

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"me"},"black":{"id":"friend"},
				"state":{"moves":"e2e4 e7e5","status":"started"}})");
			events.clear();
			client.trackSentMove("g1f3");
			// Lichess has another move than the one we sent.
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5 f1c4","status":"started"})");
			Assert::AreEqual(1, (int)events.size());
			Assert::AreEqual(QString("replaced e2e4 e7e5 f1c4"), events[0]);

			// An earlier move differs: the whole list is Lichess's.
			events.clear();
			client.handleGameStreamLine(R"({"type":"gameState","moves":"d2d4 d7d5 c2c4","status":"started"})");
			Assert::AreEqual(1, (int)events.size());
			Assert::AreEqual(QString("replaced d2d4 d7d5 c2c4"), events[0]);
		}

		TEST_METHOD(GameStream_GameFull_DropsSentMoves_Test)
		{
			LichessClient client;
			client.setUsername("me");
			QStringList events;
			recordMoveEvents(client, events);

			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"friend"},"black":{"id":"me"},
				"state":{"moves":"e2e4","status":"started"}})");
			client.trackSentMove("e7e5");   // e.g. sent just before the stream reconnected
			client.handleGameStreamLine(R"({"type":"gameFull","white":{"id":"friend"},"black":{"id":"me"},
				"state":{"moves":"e2e4","status":"started"}})");
			events.clear();
			client.handleGameStreamLine(R"({"type":"gameState","moves":"e2e4 e7e5","status":"started"})");
			Assert::AreEqual(1, (int)events.size(), L"after gameFull only Lichess's moves count");
			Assert::AreEqual(QString("move e7e5"), events[0]);
		}
	};

	// -----------------------------------------------------------------------
	// Integration tests — real HTTP via QNetworkAccessManager
	// Requires QCoreApplication (created by TEST_MODULE_INITIALIZE in
	// UciConnectorTest.cpp which shares this DLL module).
	// -----------------------------------------------------------------------
	TEST_CLASS(LichessNetworkTests)
	{
		// Spins the Qt event loop until the given flag is set or 5 s elapse.
		static void waitForSignal(bool& received, int timeoutMs = 5000)
		{
			QEventLoop loop;
			QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
			// Poll-exit: re-check every 50 ms so we don't wait the full 5 s.
			QTimer poll;
			poll.setInterval(50);
			QObject::connect(&poll, &QTimer::timeout, [&]() {
				if (received) loop.quit();
				});
			poll.start();
			loop.exec();
		}

	public:

		TEST_METHOD(ValidateToken_FakeToken_EmitsFalse)
		{
			LichessClient client;

			bool signalFired = false;
			bool okValue = true;
			QString username;

			QObject::connect(&client, &LichessClient::tokenValidated,
				[&](bool ok, QString name) {
					okValue = ok;
					username = name;
					signalFired = true;
				});

			client.validateToken("this_is_not_a_real_token_xyz");

			waitForSignal(signalFired);

			Assert::IsTrue(signalFired, L"tokenValidated signal must fire within 5 s");
			Assert::IsFalse(okValue, L"Fake token must not validate");
			Assert::IsTrue(username.isEmpty(), L"Username must be empty for rejected token");
		}

		TEST_METHOD(ValidateToken_RealToken_EmitsTrue)
		{
			const QString lichessToken = qEnvironmentVariable("LICHESS_TEST_TOKEN");
			if (lichessToken.isEmpty()) {
				Logger::WriteMessage("LICHESS_TEST_TOKEN not set — skipping real-token test");
				return;
			}

			LichessClient client;

			bool signalFired = false;
			bool okValue = false;
			QString username;

			QObject::connect(&client, &LichessClient::tokenValidated,
				[&](bool ok, QString name) {
					okValue = ok;
					username = name;
					signalFired = true;
				});

			client.validateToken(lichessToken);

			waitForSignal(signalFired);

			Assert::IsTrue(signalFired, L"tokenValidated signal must fire within 5 s");
			Assert::IsTrue(okValue, L"Real token must validate successfully");
			Assert::IsFalse(username.isEmpty(), L"Real token must return a username");
		}
	};
}