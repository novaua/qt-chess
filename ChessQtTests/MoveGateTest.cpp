#include <CppUnitTest.h>
#include <QString>
#include <vector>
#include "movegate.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	inline std::wstring ToString(const QString& q) { return q.toStdWString(); }
}

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
		int _idle = 0;

		TEST_METHOD_INITIALIZE(Init)
		{
			_gate.setClock([this] { return _now; });
			QObject::connect(&_gate, &MoveGate::release, [this](const QString& move, MoveGate::Source source) {
				_released.push_back({ move, source });
				_gate.moveShown(source == MoveGate::Source::Robot);
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

			_now = 5000;
			ready();
			Assert::AreEqual(3, (int)_released.size());
			Assert::AreEqual(QString("e2e4"), _released[0].move);
			Assert::AreEqual(QString("g1f3"), _released[2].move);
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
			_gate.tick();
			Assert::AreEqual(1, _idle);

			_now = 1000;
			_gate.moveShown(true);
			_now = 1399;
			_gate.tick();
			Assert::AreEqual(1, _idle);

			_now = 1400;
			_gate.tick();
			Assert::AreEqual(2, _idle);
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
