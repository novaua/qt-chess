#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <CppUnitTest.h>
#include "gamelibrary.h"
#include "gamesignature.h"
#include "StartPosition.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// BL-20: the signature of a kept game (GameSignature, HMAC-SHA256 with the app's key) and the game file
// it is checked on (GameLibrary::gameFileContent / readGameFile).
namespace GameSignatureTests
{
	using Kind = GameSignature::Kind;

	TEST_CLASS(GameSignatureTest)
	{
	public:
		TEST_METHOD(SignedVerifies_AChangedByteDoesnt)
		{
			const QByteArray payload = R"({"id":"g1","moves":["e2e4"],"result":"1-0"})";
			const QString sig = GameSignature::sign(Kind::Game, payload);
			Assert::IsTrue(sig.startsWith("1:"), L"key 1");
			Assert::AreEqual(2 + 64, (int)sig.size(), L"64 hex digits");
			Assert::IsTrue(GameSignature::verify(Kind::Game, payload, sig));
			Assert::IsTrue(GameSignature::isCurrent(sig));

			QByteArray edited = payload;
			edited.replace("1-0", "0-1");
			Assert::IsFalse(GameSignature::verify(Kind::Game, edited, sig), L"an edited result");
			QString flipped = sig;
			flipped[flipped.size() - 1] = flipped.back() == QLatin1Char('0') ? QLatin1Char('1') : QLatin1Char('0');
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, flipped), L"an edited signature");
		}

		// A game's signature never fits the rating or the statistics, nor the other way round.
		TEST_METHOD(EachKindHasItsOwnSignature)
		{
			const QByteArray payload = R"({"value":1500,"games":20})";
			Assert::IsFalse(GameSignature::verify(Kind::Rating, payload, GameSignature::sign(Kind::Game, payload)));
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, GameSignature::sign(Kind::Statistics, payload)));
			Assert::IsTrue(GameSignature::verify(Kind::Rating, payload, GameSignature::sign(Kind::Rating, payload)));
		}

		TEST_METHOD(UnknownKeysAndMalformedSignaturesFail)
		{
			const QByteArray payload = "x";
			const QString hex = GameSignature::sign(Kind::Game, payload).mid(2);
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, "2:" + hex), L"a key this copy doesn't know");
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, hex), L"no key number");
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, ""));
			Assert::IsFalse(GameSignature::verify(Kind::Game, payload, "1:abc"), L"too short");
			Assert::AreEqual(0, GameSignature::keyOf("x:" + hex));
			Assert::AreEqual(1, GameSignature::keyOf("1:" + hex));
		}

		// Key 1 never changes: every kept game would be set aside after an update. A fixed payload's
		// signature, worked out once (Python's hmac with the same key), pins it.
		TEST_METHOD(KeyOneIsPinned)
		{
			Assert::AreEqual(QStringLiteral("1:eed17ddfb9f91a5c739dff7ce578fc2970f23052419200216cfa5e92b338c2a0"),
			                 GameSignature::sign(Kind::Game, R"({"id":"pinned"})"));
		}

		// The record read back from its file gives the same bytes again, so its signature holds: a record
		// with everything it can carry (an analysis with fractions, Chess960, a fork, a clock, a rating).
		TEST_METHOD(AGameFileReadsBackAndVerifies)
		{
			GameRecord r;
			r.id = QStringLiteral("0b0f6c5e-1a2b-4c3d-9e8f-0123456789ab");
			r.kind = QStringLiteral("computer");
			r.level = 3;
			r.opponentName = QStringLiteral("Robot · Ünïcödé");
			r.startedAt = QDateTime(QDate(2026, 11, 18), QTime(18, 30, 15));
			r.finishedAt = r.startedAt.addSecs(1234);
			r.finished = true;
			r.result = QStringLiteral("1-0");
			r.resultText = QStringLiteral("Tester wins by checkmate");
			r.setStartPosition(Chess::StartPosition::Chess960(3));
			r.moves = { "f1g1", "f8g8", "e2e4" };
			r.fen = QStringLiteral("bqnnrrkb/pppppppp/8/8/4P3/8/PPPP1PPP/BQNNRRKB b - - 0 2");
			r.clockLimitSec = 300; r.clockIncSec = 3; r.whiteMs = 123456789012; r.blackMs = 7;
			r.forkOfId = QStringLiteral("other"); r.forkPly = 2;
			r.rated = true; r.ratingBefore = 1234; r.ratingChange = -17;
			r.analysis = QJsonObject{
				{ "depth", 14 },
				{ "evals", QJsonArray{ 0.35, -1.27, 123.456789, 1e-7, 0.1, -0.0, 1.0 / 3.0, 32000 } },
				{ "best", QJsonArray{ "e2e4", "e7e5" } },
			};

			QTemporaryDir dir;
			const QString path = dir.path() + "/" + r.id + ".json";
			{
				QFile f(path);
				Assert::IsTrue(f.open(QIODevice::WriteOnly));
				f.write(GameLibrary::gameFileContent(r));
			}
			QString why;
			bool resign = true;
			const auto read = GameLibrary::readGameFile(path, &why, &resign);
			Assert::IsTrue(read.has_value(), why.toStdWString().c_str());
			Assert::IsFalse(resign, L"signed with the current key");
			Assert::AreEqual(r.opponentName, read->opponentName);
			Assert::AreEqual(3, read->startPosition().Number());
			Assert::AreEqual((qint64)123456789012, read->whiteMs);
			// Written again from what was read: the same bytes (the canonical JSON holds).
			Assert::IsTrue(GameLibrary::gameFileContent(*read) == GameLibrary::gameFileContent(r));
		}
	};
}
