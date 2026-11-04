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
		}

		// corresClockView.ts: days and hours, then hours and minutes, then minutes and seconds.
		TEST_METHOD(FormatCorrespondence_AsLichess)
		{
			const qint64 hour = 3600000;
			areEqual("3 days", GameClock::formatCorrespondence(72 * hour));
			areEqual("2 days 23 hours", GameClock::formatCorrespondence(72 * hour - 1));
			areEqual("2 days 1 hour", GameClock::formatCorrespondence(49 * hour));
			areEqual("One day", GameClock::formatCorrespondence(24 * hour));
			areEqual("One day 5 hours", GameClock::formatCorrespondence(29 * hour));
			areEqual("23:59", GameClock::formatCorrespondence(24 * hour - 1));
			areEqual("05:30", GameClock::formatCorrespondence(5 * hour + 1800000));
			areEqual("01:00", GameClock::formatCorrespondence(hour));
			areEqual("59:59", GameClock::formatCorrespondence(hour - 1));
			areEqual("00:09", GameClock::formatCorrespondence(9400)); // no tenths
			areEqual("00:00", GameClock::formatCorrespondence(0));
		}

		// Runs from the start, never low, no sound, and Lichess ends the game.
		TEST_METHOD(Correspondence_RunsFromTheStart_NeverLow)
		{
			_clock.setLowTimeSides(true, true);
			_clock.startCorrespondence(3 * 86400000LL);
			areEqual("3 days", _clock.whiteText());
			_clock.setSnapshot(30000, 3 * 86400000LL, true, true);
			Assert::IsTrue(_clock.whiteRunning());
			Assert::IsFalse(_clock.whiteLow());
			passMs(30000);
			areEqual("00:00", _clock.whiteText());
			Assert::IsTrue(_lowTimes.empty());
			Assert::IsTrue(_flags.empty());
		}

		// Never as a real-time clock, not even for a moment.
		TEST_METHOD(Correspondence_StartsShowingDays)
		{
			std::vector<QString> shown;
			QObject::connect(&_clock, &GameClock::changed, [&] { shown.push_back(_clock.whiteText()); });
			_clock.startCorrespondence(3 * 86400000LL);
			Assert::AreEqual(size_t(1), shown.size());
			areEqual("3 days", shown[0]);
		}

		TEST_METHOD(Correspondence_MoveSwitchesTheClock_EvenTheFirst)
		{
			_clock.startCorrespondence(86400000);
			_clock.moveMade(true, 1);
			Assert::IsTrue(_clock.blackRunning());
			_clock.setTurn(true, 0);
			Assert::IsTrue(_clock.whiteRunning());
		}

		// The timer wakes when the text changes: an hour, a minute, a second.
		TEST_METHOD(Correspondence_WakesWhenTheTextChanges)
		{
			_clock.startCorrespondence(3 * 86400000LL);
			_clock.setSnapshot(2 * 86400000LL + 1800000, 0, true, true);
			Assert::AreEqual(qint64(1800001), _clock.nextTickMs());
			_clock.setSnapshot(3600000 + 20000, 0, true, true);
			Assert::AreEqual(qint64(20001), _clock.nextTickMs());
			_clock.setSnapshot(1500, 0, true, true);
			Assert::AreEqual(qint64(501), _clock.nextTickMs());
		}

		// A new real-time game after a correspondence one is an ordinary clock again.
		TEST_METHOD(Start_AfterCorrespondence)
		{
			_clock.startCorrespondence(86400000);
			_clock.start(60000, 0, false);
			areEqual("01:00", _clock.whiteText());
			_clock.moveMade(true, 1);
			Assert::IsFalse(_clock.blackRunning());
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

		// A move and Undo over and over must not add time: the increment goes, the time spent stays spent.
		TEST_METHOD(MoveTakenBack_TheIncrementGoes)
		{
			_clock.start(60000, 5000);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			passMs(1000);
			_clock.moveMade(true, 3);  // 59 s + 5 s
			passMs(2000);
			_clock.moveMade(false, 4); // 58 s + 5 s
			_clock.moveTakenBack(false, 4);
			_clock.moveTakenBack(true, 3);
			_clock.setTurn(true, 2);
			Assert::AreEqual(qint64(59000), _clock.remainingMs(true));
			Assert::AreEqual(qint64(58000), _clock.remainingMs(false));
		}

		TEST_METHOD(MoveTakenBack_TheFirstTwoMovesGotNone)
		{
			_clock.start(60000, 5000);
			_clock.moveMade(true, 1);
			_clock.moveMade(false, 2);
			_clock.moveTakenBack(false, 2);
			_clock.moveTakenBack(true, 1);
			Assert::AreEqual(qint64(60000), _clock.remainingMs(true));
			Assert::AreEqual(qint64(60000), _clock.remainingMs(false));
		}

		// The increment had saved the player, the time is nearly gone again: Undo doesn't flag.
		TEST_METHOD(MoveTakenBack_LeavesTheLastSecond)
		{
			_clock.start(60000, 5000);
			_clock.setSnapshot(3000, 60000, true, false);
			_clock.moveTakenBack(true, 3);
			Assert::AreEqual(qint64(1000), _clock.remainingMs(true));
			_clock.setSnapshot(400, 60000, true, false);
			_clock.moveTakenBack(true, 3);
			Assert::AreEqual(qint64(400), _clock.remainingMs(true));
			Assert::IsTrue(_flags.empty());
		}

		// The flag belongs to the game that ended: a next game without a clock must have its Undo.
		TEST_METHOD(Stop_ForgetsTheFlag)
		{
			_clock.start(60000, 0);
			_clock.setSnapshot(0, 60000, true, true);
			Assert::IsTrue(_clock.timeUp());
			_clock.stop();
			Assert::IsFalse(_clock.timeUp());
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
			_clock.setSnapshot(120, 60000, true, true);
			Assert::AreEqual(qint64(60), _clock.moveBudgetMs(true)); // the 0.1 s must not flag it
			_clock.start(120000, 1000);
			_clock.setSnapshot(300, 60000, true, true);
			Assert::AreEqual(qint64(150), _clock.moveBudgetMs(true)); // nor the increment it hasn't got yet
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
