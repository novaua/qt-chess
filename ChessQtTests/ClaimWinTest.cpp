#include <CppUnitTest.h>
#include "claimwin.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// ClaimWin (BL-13 review): the opponent left; the win can be claimed once Lichess's countdown is over
// and only on the opponent's turn. The seconds are moved by hand with tick().
namespace ClaimWinTests
{
	TEST_CLASS(ClaimWinTest)
	{
		ClaimWin _claim;
		int _goneSignals = 0;
		int _secondsSignals = 0;
		int _canClaimSignals = 0;

		TEST_METHOD_INITIALIZE(Init)
		{
			QObject::connect(&_claim, &ClaimWin::goneChanged, [this] { ++_goneSignals; });
			QObject::connect(&_claim, &ClaimWin::secondsLeftChanged, [this] { ++_secondsSignals; });
			QObject::connect(&_claim, &ClaimWin::canClaimChanged, [this] { ++_canClaimSignals; });
		}

		TEST_METHOD(NotGoneCantClaim)
		{
			_claim.setOpponentToMove(true);
			Assert::IsFalse(_claim.gone());
			Assert::IsFalse(_claim.canClaim());
			Assert::AreEqual(-1, _claim.secondsLeft());
		}

		TEST_METHOD(CountdownThenClaim)
		{
			_claim.setOpponentToMove(true);
			_claim.setGone(true, 2);
			Assert::IsTrue(_claim.gone());
			Assert::IsFalse(_claim.canClaim());
			Assert::AreEqual(2, _claim.secondsLeft());
			_claim.tick();
			Assert::AreEqual(1, _claim.secondsLeft());
			Assert::IsFalse(_claim.canClaim());
			_claim.tick();
			Assert::AreEqual(0, _claim.secondsLeft());
			Assert::IsTrue(_claim.canClaim());
			Assert::AreEqual(1, _canClaimSignals);
			_claim.tick();   // stays at 0
			Assert::AreEqual(0, _claim.secondsLeft());
			Assert::AreEqual(3, _secondsSignals);   // -1 → 2 → 1 → 0
		}

		TEST_METHOD(NotSaidMeansNow)
		{
			_claim.setOpponentToMove(true);
			_claim.setGone(true);
			Assert::IsTrue(_claim.canClaim());
		}

		// Lichess says 0 on the player's turn too, but refuses the claim then.
		TEST_METHOD(OnlyOnTheOpponentsTurn)
		{
			_claim.setGone(true, 0);
			Assert::IsFalse(_claim.canClaim());
			_claim.setOpponentToMove(true);
			Assert::IsTrue(_claim.canClaim());
			_claim.setOpponentToMove(false);
			Assert::IsFalse(_claim.canClaim());
			Assert::AreEqual(2, _canClaimSignals);
		}

		// A new countdown while still gone isn't a new departure (a closed message stays closed).
		TEST_METHOD(NewCountdownKeepsGone)
		{
			_claim.setGone(true, 12);
			_claim.setGone(true, 5);
			Assert::AreEqual(1, _goneSignals);
			Assert::AreEqual(5, _claim.secondsLeft());
		}

		TEST_METHOD(BackClearsTheCountdown)
		{
			_claim.setOpponentToMove(true);
			_claim.setGone(true, 0);
			_claim.setGone(false, 7);
			Assert::IsFalse(_claim.gone());
			Assert::IsFalse(_claim.canClaim());
			Assert::AreEqual(-1, _claim.secondsLeft());
			Assert::AreEqual(2, _goneSignals);
			Assert::AreEqual(2, _canClaimSignals);
		}
	};
}
