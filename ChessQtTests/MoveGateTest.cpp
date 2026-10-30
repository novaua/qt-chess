#include <CppUnitTest.h>
#include <QString>
#include <QStringList>
#include <vector>
#include "movegate.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// MoveGate (BL-14): a move from outside is released only when the board is idle, and a robot move
// also waits robotMinMs after the player's move. The clock is a member the test moves by hand; the
// release handler plays the move and calls moveShown(), as ChessConnector will.
namespace MoveGateTests
{
	TEST_CLASS(MoveGateTest)
	{
		struct Released {
			QString move;
			MoveGate::Source source;
		};

		qint64 _now = 1000;
		MoveGate _gate;
		std::vector<Released> _released;
		QStringList _batch;
		int _batches = 0;
		int _idle = 0;
		bool _playOnRelease = true; // as ChessConnector will: play it, then moveShown()

		TEST_METHOD_INITIALIZE(Init)
		{
			_gate.setClock([this] { return _now; });
			QObject::connect(&_gate, &MoveGate::release, [this](const QString& move, MoveGate::Source source) {
				_released.push_back({ move, source });
				if (_playOnRelease)
					_gate.moveShown(source == MoveGate::Source::Robot);
			});
			QObject::connect(&_gate, &MoveGate::releaseBatch, [this](const QStringList& moves) {
				++_batches;
				_batch = moves;
				if (_playOnRelease)
					_gate.moveShown();
			});
			QObject::connect(&_gate, &MoveGate::idle, [this] { ++_idle; });
		}

		void setTiming(int slide, int pause, int robotMin)
		{
			_gate.setTiming({ slide, pause, robotMin });
		}

		void ready()
		{
			_gate.setStageReady(true);
		}

		static bool isRobot(const Released& move)
		{
			return move.source == MoveGate::Source::Robot;
		}

	public:
		// The slide and its pause end at 1400, but the robot may not answer before 1700.
		TEST_METHOD(RobotMoveWaitsForTheSlideThePauseAndTheRobotMinimum)
		{
			setTiming(250, 150, 700);
			ready();
			_now = 1000;
			_gate.moveShown(false); // the player's move
			_now = 1200;
			_gate.push("e7e5", MoveGate::Source::Robot);

			_now = 1399;
			_gate.tick();
			Assert::IsTrue(_released.empty());

			_now = 1400; // the slide is over, the robot minimum is not
			_gate.tick();
			Assert::IsTrue(_released.empty());

			_now = 1700;
			_gate.tick();
			Assert::AreEqual(1, (int)_released.size());
			Assert::AreEqual(QString("e7e5"), _released[0].move);
			Assert::IsTrue(isRobot(_released[0]));
		}

		// The opponent's move online waits for our slide only, not for robotMinMs.
		TEST_METHOD(OnlineMoveDoesNotWaitForTheRobotMinimum)
		{
			setTiming(250, 150, 700);
			ready();
			_now = 1000;
			_gate.moveShown(false);
			_gate.push("d1h5", MoveGate::Source::Online);

			_now = 1399;
			_gate.tick();
			Assert::IsTrue(_released.empty());

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(1, (int)_released.size());
			Assert::IsFalse(isRobot(_released[0]));
		}

		TEST_METHOD(StageNotReadyHoldsTheMove)
		{
			setTiming(250, 150, 400);
			_gate.push("e7e5", MoveGate::Source::Robot);
			_now = 5000;
			_gate.tick();
			Assert::IsTrue(_released.empty());
			Assert::IsTrue(_gate.holding());

			ready();
			Assert::AreEqual(1, (int)_released.size());
			Assert::IsFalse(_gate.holding());
		}

		TEST_METHOD(ClearDropsTheMove)
		{
			setTiming(250, 150, 400);
			_gate.push("e7e5", MoveGate::Source::Robot);
			_gate.clear();
			Assert::IsFalse(_gate.holding());

			_now = 5000;
			ready();
			_gate.tick();
			Assert::IsTrue(_released.empty());
		}

		// One at a time: the second move waits for the first one's slide and pause.
		TEST_METHOD(TwoOnlineMovesComeOutOneAfterTheOther)
		{
			setTiming(250, 150, 400);
			ready();
			_now = 1000;
			_gate.push("e2e4", MoveGate::Source::Online);
			Assert::AreEqual(1, (int)_released.size()); // the board was idle

			_gate.push("e7e5", MoveGate::Source::Online);
			Assert::AreEqual(1, (int)_released.size());

			_now = 1399;
			_gate.tick();
			Assert::AreEqual(1, (int)_released.size());

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(2, (int)_released.size());
			Assert::AreEqual(QString("e7e5"), _released[1].move);
		}

		// A Lichess resync after a reconnect: the whole batch at once, not one slide each.
		TEST_METHOD(MoreThanTwoMovesAreReleasedTogether)
		{
			setTiming(250, 150, 400);
			_gate.push("e2e4", MoveGate::Source::Online);
			_gate.push("e7e5", MoveGate::Source::Online);
			_gate.push("g1f3", MoveGate::Source::Online);
			Assert::IsTrue(_released.empty()); // the stage is not ready yet
			Assert::AreEqual(0, _batches);

			_now = 5000;
			ready();
			Assert::AreEqual(0, (int)_released.size());
			Assert::AreEqual(1, _batches);
			Assert::AreEqual(3, (int)_batch.size());
			Assert::AreEqual(QString("e2e4"), _batch[0]);
			Assert::AreEqual(QString("g1f3"), _batch[2]);
		}

		// Nothing of the abandoned game holds the new one: clear() drops its pacing too.
		TEST_METHOD(ClearResetsThePacing)
		{
			setTiming(250, 150, 700);
			ready();
			_now = 1000;
			_gate.moveShown(false); // the board would be busy until 1700 for the robot
			_gate.clear();
			Assert::IsTrue(_gate.isIdle());

			_gate.push("e7e5", MoveGate::Source::Robot);
			Assert::AreEqual(1, (int)_released.size());
		}

		// Reduce animations while a slide's window is running: the new, shorter one applies at once.
		TEST_METHOD(SetTimingShortensTheWindowUnderWay)
		{
			setTiming(250, 150, 50);
			ready();
			_now = 1000;
			_gate.moveShown(false); // busy until 1400
			_gate.push("d1h5", MoveGate::Source::Online);

			_now = 1200;
			_gate.tick();
			Assert::IsTrue(_released.empty());

			setTiming(0, 100, 50); // busy until 1100, already over
			Assert::AreEqual(1, (int)_released.size());
			Assert::IsFalse(isRobot(_released[0]));
		}

		// The receiver may apply a move as a position reset and not call moveShown(): the gate still
		// keeps the board busy for a slide and a pause and settles afterwards.
		TEST_METHOD(AReleaseWithoutMoveShownStillSettles)
		{
			setTiming(250, 150, 400);
			ready();
			_now = 1000;
			_playOnRelease = false;
			_gate.push("e2e4", MoveGate::Source::Online);
			Assert::AreEqual(1, (int)_released.size());
			Assert::IsFalse(_gate.isIdle());

			_now = 1399;
			_gate.tick();
			Assert::AreEqual(1, _idle);

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(2, _idle);
		}

		// Reduce animations: no slide time, but the beat after it stays.
		TEST_METHOD(ReduceAnimationsStillKeepsThePause)
		{
			setTiming(0, 150, 400);
			ready();
			_now = 1000;
			_gate.moveShown(false);
			_gate.push("e7e5", MoveGate::Source::Online);

			_now = 1149;
			_gate.tick();
			Assert::IsTrue(_released.empty());

			_now = 1150;
			_gate.tick();
			Assert::AreEqual(1, (int)_released.size());
		}

		// idle once the page has settled, again after each shown move's slide — but not twice.
		TEST_METHOD(IdleWhenNothingWaitsAndTheBoardHasSettled)
		{
			setTiming(250, 150, 400);
			ready();
			Assert::AreEqual(1, _idle);
			Assert::IsTrue(_gate.isIdle());
			_gate.tick();
			Assert::AreEqual(1, _idle);

			_now = 1000;
			_gate.moveShown(true);
			Assert::IsFalse(_gate.isIdle());
			_now = 1399;
			_gate.tick();
			Assert::AreEqual(1, _idle);

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(2, _idle);
			Assert::IsTrue(_gate.isIdle());
		}

		TEST_METHOD(IdleAfterTheLastReleasedMove)
		{
			setTiming(250, 150, 400);
			ready();
			_now = 1000;
			_gate.push("e7e5", MoveGate::Source::Robot);
			Assert::AreEqual(1, (int)_released.size());

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(2, _idle);
		}
	};
}
