#include <CppUnitTest.h>
#include "gamelibrary.h"
#include "gamepgn.h"
#include "EngineLevel.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// A game as PGN (BL-19 step 3): the tags of a kept game (what kind of game, the robot's Elo, the
// clock) and its moves in SAN, as a Games card's Export PGN writes them.
namespace GamePgnTests
{
	static GameRecord record(const QString& kind, bool playerWhite, const QStringList& moves)
	{
		GameRecord r;
		r.kind = kind;
		r.playerPlaysWhite = playerWhite;
		r.opponentName = kind == QLatin1String("computer") ? QStringLiteral("Robot") : QStringLiteral("Bob");
		r.level = kind == QLatin1String("computer") ? 4 : 0;
		r.startedAt = QDateTime(QDate(2026, 11, 14), QTime(18, 0));
		r.moves = moves;
		return r;
	}

	static bool has(const QString& text, const QString& part) { return text.contains(part); }

	TEST_CLASS(GamePgnTest)
	{
	public:
		TEST_METHOD(TwoPlayerGame_TagsMovesAndResult)
		{
			GameRecord r = record("local", false, { "f2f3", "e7e5", "g2g4", "d8h4" });
			r.finished = true;
			r.result = QStringLiteral("0-1");
			const auto pgn = GamePgn::ofRecord(r, QStringLiteral("Alice"));
			Assert::IsTrue(pgn.has_value());
			const QString text = pgn->text();
			Assert::IsTrue(has(text, "[Event \"Casual two-player game\"]"), text.toStdWString().c_str());
			Assert::IsTrue(has(text, "[Date \"2026.11.14\"]"));
			Assert::IsTrue(has(text, "[White \"Bob\"]"), L"the player plays Black");
			Assert::IsTrue(has(text, "[Black \"Alice\"]"));
			Assert::IsTrue(has(text, "[Result \"0-1\"]"));
			Assert::IsTrue(has(text, "[TimeControl \"-\"]"), L"no clock");
			Assert::IsFalse(has(text, "Elo"));
			Assert::IsTrue(has(text, "1. f3 e5 2. g4 Qh4# 0-1"), text.toStdWString().c_str());
			Assert::AreEqual(QStringLiteral("Bob vs Alice 2026-11-14.pgn"), pgn->fileName());
		}

		// LI-01A: as lichess.org writes a Chess960 game: Variant, SetUp and the start FEN; castling O-O.
		TEST_METHOD(Chess960_VariantSetUpAndFen)
		{
			GameRecord r = record("local", true, {});
			r.setStartPosition(Chess::StartPosition::Chess960(3));
			r.moves = { "f1g1", "f8g8" };
			const auto pgn = GamePgn::ofRecord(r, QStringLiteral("Alice"));
			Assert::IsTrue(pgn.has_value());
			const QString text = pgn->text();
			Assert::IsTrue(has(text, "[Variant \"Chess960\"]"), text.toStdWString().c_str());
			Assert::IsTrue(has(text, "[SetUp \"1\"]"));
			Assert::IsTrue(has(text, "[FEN \"bqnnrkrb/pppppppp/8/8/8/8/PPPPPPPP/BQNNRKRB w KQkq - 0 1\"]"));
			Assert::IsTrue(has(text, "1. O-O O-O *"), text.toStdWString().c_str());

			const auto standard = GamePgn::ofRecord(record("local", true, { "e2e4" }), QStringLiteral("Alice"));
			Assert::IsFalse(has(standard->text(), "Variant"));
			Assert::IsFalse(has(standard->text(), "FEN"));
		}

		TEST_METHOD(RobotGame_EventLevelEloAndClock)
		{
			GameRecord r = record("computer", true, { "e2e4", "e7e5" });
			r.clockLimitSec = 300;
			r.clockIncSec = 3;
			const auto pgn = GamePgn::ofRecord(r, QStringLiteral("Alice"));
			Assert::IsTrue(pgn.has_value());
			const Chess::EngineLevel level(4);
			Assert::AreEqual(QStringLiteral("Casual game vs the robot, level 4 (%1)").arg(QString::fromUtf8(level.Name())), pgn->event());
			Assert::AreEqual(0, pgn->whiteElo(), L"the player's Elo: only in a rated game");
			Assert::AreEqual(level.DisplayElo(), pgn->blackElo(), L"the robot plays Black");
			Assert::AreEqual(QStringLiteral("300+3"), pgn->timeControl());
			const QString text = pgn->text();
			Assert::IsTrue(has(text, QStringLiteral("[BlackElo \"%1\"]").arg(level.DisplayElo())));
			Assert::IsTrue(has(text, "[TimeControl \"300+3\"]"));
			Assert::IsTrue(has(text, "[Result \"*\"]"), L"unfinished");
			Assert::IsTrue(has(text, "1. e4 e5 *"), text.toStdWString().c_str());
		}

		// A rated game (step 5): the player's rating before it.
		TEST_METHOD(RatedRobotGame_PlayerElo)
		{
			GameRecord r = record("computer", true, { "e2e4", "e7e5" });
			r.rated = true;
			r.ratingBefore = 1234;
			r.ratingChange = 12;
			const auto pgn = GamePgn::ofRecord(r, QStringLiteral("Alice"));
			Assert::AreEqual(1234, pgn->whiteElo(), L"the player plays White");
			Assert::AreEqual(Chess::EngineLevel(4).DisplayElo(), pgn->blackElo());
			Assert::IsTrue(has(pgn->text(), "[WhiteElo \"1234\"]"));
		}

		TEST_METHOD(DamagedRecord_NoPgn)
		{
			Assert::IsFalse(GamePgn::ofRecord(record("local", true, { "e2e4", "e7e5", "e4e6" }), "Alice").has_value(),
				L"a move that can't be played");
		}

		TEST_METHOD(LichessGame_SiteAndNoTimeControl)
		{
			GamePgn pgn("Alice", "Bob", { "e4" });
			pgn.setTimeControl(180, 2);
			pgn.setLichessGame("abcd1234");
			const QString text = pgn.text();
			Assert::IsTrue(has(text, "[Site \"https://lichess.org/abcd1234\"]"));
			Assert::IsFalse(has(text, "TimeControl"), L"Lichess's own PGN has it");
			Assert::IsTrue(has(text, "[Date \"????.??.??\"]"), L"no date given");
		}

		TEST_METHOD(SetMoves_PositionAfterThem)
		{
			GameRecord r;
			Assert::IsTrue(r.setMoves({ "e2e4" }));
			Assert::AreEqual(1, r.plyCount());
			Assert::IsTrue(r.fen.startsWith("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b"), r.fen.toStdWString().c_str());
			Assert::IsFalse(r.setMoves({ "e2e5" }), L"not a legal move");
			Assert::AreEqual(1, r.plyCount(), L"unchanged");
		}
	};
}
