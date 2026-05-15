#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <CppUnitTest.h>
#include "LichessClient.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Teach CppUnitTest how to display QString in assertion failure messages.
namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	inline std::wstring ToString(const QString& q) { return q.toStdWString(); }
}

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