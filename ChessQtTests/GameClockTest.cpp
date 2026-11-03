#include <CppUnitTest.h>
#include <QString>
#include <vector>
#include "gameclock.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// GameClock (LI-02): the two clocks of a game, as on lichess.org. The time is a member the test moves
// by hand, then tick() (the clock's own timer does that in the app).
namespace GameClockTests
{
	TEST_CLASS(GameClockTest)
	{
		qint64 _now = 1000;
		GameClock _clock;
		std::vector<bool> _flags;    // flagged(white)
		std::vector<bool> _lowTimes; // lowTime(white)

		TEST_METHOD_INITIALIZE(Init)
		{
			_clock.setClock([this] { return _now; });
			QObject::connect(&_clock, &GameClock::flagged, [this](bool white) { _flags.push_back(white); });
			QObject::connect(&_clock, &GameClock::lowTime, [this](bool white) { _lowTimes.push_back(white); });
		}

		void passMs(qint64 ms)
		{
			_now += ms;
			_clock.tick();
		}

		static void areEqual(const char* expected, const QString& actual)
		{
			Assert::AreEqual(QString::fromUtf8(expected).toStdWString(), actual.toStdWString());
		}

	public:
		TEST_METHOD(Format_AsLichess)
		{
			areEqual("03:00", GameClock::format(180000));
			areEqual("02:59", GameClock::format(179999));   // floored, like Lichess
			areEqual("00:45", GameClock::format(45000));
			areEqual("00:10", GameClock::format(10000));
			areEqual("00:09.9", GameClock::format(9999));   // tenths below 10 s
			areEqual("00:00.4", GameClock::format(450));
			areEqual("00:00.0", GameClock::format(0));
			areEqual("00:00.0", GameClock::format(-30));
			areEqual("01:30:00", GameClock::format(90 * 60000));
			areEqual("23:59:59", GameClock::format(86399999));
			areEqual("1 day", GameClock::format(86400000));
			areEqual("3 days", GameClock::format(3 * 86400000LL + 5000));
		}

		TEST_METHOD(NoClock_ShowsNothing)
		{
			Assert::IsFalse(_clock.enabled());
			areEqual("", _clock.whiteText());
			_clock.start(60000, 0);
			_clock.stop();
			Assert::IsFalse(_clock.enabled());
			areEqual("", _clock.blackText());
		}

		// Nothing runs until both sides have moved; then White's clock.
		TEST_METHOD(RunsOnlyAfterBothFirstMoves)
		{
			_clock.start(180000, 2000);
			areEqual("03:00", _clock.whiteText());
			passMs(5000);
			_clock.moveMade(true, 1);
			passMs(7000);
			Assert::IsFalse(_clock.whiteRunning() || _clock.blackRunning());
			_clock.moveMade(false, 2);
			// Neither first move gets the increment: neither clock was running.
			Assert::AreEqual(qint64(180000), _clock.remainingMs(true));
			Assert::AreEqual(qint64(180000), _clock.remainingMs(false));
			Assert::IsTrue(_clock.whiteRunning());
			passMs(1500);
			Assert::AreEqual(qint64(178500), _clock.remainingMs(true));
			areEqual("02:58", _clock.whiteText());
		}

		TEST_METHOD(Increment_ToTheMoverWhoseClockRan)
		{
			_clock.start(60000, 2000);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(3000);
			_clock.moveMade(true, 3); // 60 - 3 + 2
			Assert::AreEqual(qint64(59000), _clock.remainingMs(true));
			Assert::IsTrue(_clock.blackRunning());
			passMs(10000);
			_clock.moveMade(false, 4); // 60 - 10 + 2
			Assert::AreEqual(qint64(52000), _clock.remainingMs(false));
			Assert::AreEqual(qint64(59000), _clock.remainingMs(true));
		}

		TEST_METHOD(Flag_OnceWhenTheTimeIsUp)
		{
			_clock.start(10000, 0);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(9999);
			Assert::AreEqual(size_t(0), _flags.size());
			areEqual("00:00.0", _clock.whiteText());
			passMs(1);
			Assert::AreEqual(size_t(1), _flags.size());
			Assert::IsTrue(_flags[0]);
			Assert::IsTrue(_clock.timeUp());
			Assert::IsFalse(_clock.whiteRunning()); // stopped at 0
			passMs(5000);
			Assert::AreEqual(size_t(1), _flags.size());
			_clock.moveMade(true, 3); // a move after the flag changes nothing
			Assert::AreEqual(qint64(0), _clock.remainingMs(true));
		}

		// The timer can be late: a move after the time ran out doesn't count, and gets no increment.
		TEST_METHOD(LateMove_TheFlagStands)
		{
			_clock.start(10000, 2000);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			_now += 10050; // no tick
			_clock.moveMade(true, 3);
			Assert::AreEqual(size_t(1), _flags.size());
			Assert::IsTrue(_flags[0]);
			Assert::AreEqual(qint64(0), _clock.remainingMs(true));
			Assert::IsFalse(_clock.blackRunning());
		}

		TEST_METHOD(LateTakeback_TheFlagStands)
		{
			_clock.start(10000, 0);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			_now += 10050;
			_clock.setTurn(false, 2);
			Assert::AreEqual(size_t(1), _flags.size());
			Assert::IsFalse(_clock.blackRunning());
		}

		// A Lichess game: Lichess ends it, the clock just stays at zero, and its timer sleeps.
		TEST_METHOD(OnlineClock_NeverFlags)
		{
			_clock.start(10000, 0, false);
			_clock.setSnapshot(500, 8000, true, true);
			Assert::IsTrue(_clock.nextTickMs() > 0);
			passMs(2000);
			Assert::AreEqual(qint64(0), _clock.remainingMs(true));
			Assert::AreEqual(size_t(0), _flags.size());
			Assert::AreEqual(qint64(0), _clock.nextTickMs());
		}

		// Lichess's times are as of the state; the side to move's clock counts from then.
		TEST_METHOD(Snapshot_ThenTheRunningSideCounts)
		{
			_clock.start(600000, 5000, false);
			_clock.setSnapshot(512345, 498000, false, true);
			Assert::IsTrue(_clock.blackRunning());
			passMs(2000);
			Assert::AreEqual(qint64(496000), _clock.remainingMs(false));
			Assert::AreEqual(qint64(512345), _clock.remainingMs(true));
			_clock.setSnapshot(512345, 498000, true, false); // e.g. the first moves: nothing runs
			passMs(2000);
			Assert::AreEqual(qint64(512345), _clock.remainingMs(true));
		}

		TEST_METHOD(PauseAndResume)
		{
			_clock.start(60000, 0);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(1000);
			_clock.pause();
			Assert::IsFalse(_clock.whiteRunning());
			passMs(30000);
			Assert::AreEqual(qint64(59000), _clock.remainingMs(true));
			_clock.resume();
			passMs(1000);
			Assert::AreEqual(qint64(58000), _clock.remainingMs(true));
		}

		// After a takeback, the side to move runs (from ply 2), with the times as they were.
		TEST_METHOD(SetTurn_AfterATakeback)
		{
			_clock.start(60000, 0);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(4000);
			_clock.moveMade(true, 3);
			passMs(1000);
			_clock.setTurn(true, 2); // two moves taken back: White to move again
			Assert::IsTrue(_clock.whiteRunning());
			Assert::AreEqual(qint64(59000), _clock.remainingMs(false));
			_clock.setTurn(true, 0);
			Assert::IsFalse(_clock.whiteRunning() || _clock.blackRunning());
		}

		// Lichess: an eighth of the start time, at least 10 s, at most 60 s; below a minute a fifth,
		// at least 2 s.
		TEST_METHOD(LowTime_Line)
		{
			_clock.start(5000, 0);
			Assert::AreEqual(qint64(2000), _clock.lowTimeMs());
			_clock.start(30000, 0);
			Assert::AreEqual(qint64(6000), _clock.lowTimeMs());
			_clock.start(60000, 0);
			Assert::AreEqual(qint64(10000), _clock.lowTimeMs());
			_clock.start(180000, 0);
			Assert::AreEqual(qint64(22500), _clock.lowTimeMs());
			_clock.start(1800000, 0);
			Assert::AreEqual(qint64(60000), _clock.lowTimeMs());
		}

		// Once below the line for a watched side; again only after an increment lifted it above 1.5
		// times the line and 20 s have passed.
		TEST_METHOD(LowTime_OnceForTheWatchedSide)
		{
			_clock.setLowTimeSides(true, false);
			_clock.start(60000, 20000);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(49999);
			Assert::AreEqual(size_t(0), _lowTimes.size());
			Assert::IsFalse(_clock.whiteLow());
			passMs(2);
			Assert::AreEqual(size_t(1), _lowTimes.size());
			Assert::IsTrue(_clock.whiteLow());
			passMs(1000);
			Assert::AreEqual(size_t(1), _lowTimes.size());
			_clock.moveMade(true, 3);  // 9 s + 20 s: above 15 s again
			passMs(15000);              // Black thinks (not watched)
			_clock.moveMade(false, 4);
			passMs(19000);              // White below 10 s again, 35 s after the first sound
			Assert::AreEqual(size_t(2), _lowTimes.size());
			Assert::IsTrue(_lowTimes[1]);
		}

		// Continue: the times of a saved game arrive while paused; the sound waits for the side's clock.
		TEST_METHOD(LowTime_OnlyWhileThatSidesClockRuns)
		{
			_clock.setLowTimeSides(true, false);
			_clock.start(60000, 0);
			_clock.pause();
			_clock.setSnapshot(8000, 30000, false, true);
			_clock.resume();
			Assert::AreEqual(size_t(0), _lowTimes.size()); // Black to move
			Assert::IsTrue(_clock.whiteLow());             // red all the same
			_clock.setTurn(true, 10);
			Assert::AreEqual(size_t(1), _lowTimes.size());
		}

		// The robot can't flag: a thirtieth of its time plus half the increment, at least 0.1 s.
		TEST_METHOD(MoveBudget)
		{
			Assert::AreEqual(qint64(0), _clock.moveBudgetMs(true)); // no clock
			_clock.start(180000, 2000);
			Assert::AreEqual(qint64(7000), _clock.moveBudgetMs(false));
			_clock.start(60000, 0);
			_clock.setSnapshot(1500, 60000, true, true);
			Assert::AreEqual(qint64(100), _clock.moveBudgetMs(true));
		}

		TEST_METHOD(Changed_WhenTheShownTimeChanges)
		{
			int changes = 0;
			QObject::connect(&_clock, &GameClock::changed, [&changes] { ++changes; });
			_clock.start(60000, 0);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(100);   // 00:59
			changes = 0;
			passMs(400);   // still 00:59
			passMs(400);
			Assert::AreEqual(0, changes);
			passMs(300);   // 00:58
			Assert::AreEqual(1, changes);
		}
	};
}
