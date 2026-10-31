#include "stdafx.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include <QElapsedTimer>
#include <QThread>
#include <atomic>
#include "Game.h"
#include "ChessEnginePlayer.h"
#include "engineworker.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

// The engine player works on a copy of the moves and returns its move; it never plays it
// (BL-09: the GUI thread owns the Game). These tests run the real engine, like UciConnectorTest.
namespace EnginePlayerTests
{
	static MovesHistory Play(const vector<string>& moves)
	{
		Game game;
		game.Restart();
		for (const auto& m : moves)
			game.DoMove(Move::Parse(m));
		return game.GetGameRecord();
	}

	// White to move, Qh5 mates.
	static const vector<string> MateInOne = { "e2e4", "f7f6", "d2d4", "g7g5" };
	// The same position after 12 plies, so FenWindow sends a FEN at ply 8 plus 4 moves.
	static const vector<string> LongMateInOne = { "g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1", "f6g8",
	                                              "e2e4", "f7f6", "d2d4", "g7g5" };

	TEST_CLASS(ChessEnginePlayerTests)
	{
		static string FindMove(PositionMode mode, const vector<string>& moves)
		{
			ChessEnginePlayer player(EngineLevel{ 5 }, mode);
			auto move = player.FindMove(Play(moves));
			player.KillEngine();
			return move;
		}

	public:
		TEST_METHOD(FindMove_StartPos_FindsMate_Test)
		{
			Assert::AreEqual(string("d1h5"), FindMove(PositionMode::StartPos, MateInOne));
		}

		TEST_METHOD(FindMove_PureFen_FindsMate_Test)
		{
			Assert::AreEqual(string("d1h5"), FindMove(PositionMode::PureFen, LongMateInOne));
		}

		TEST_METHOD(FindMove_FenWindow_FindsMate_Test)
		{
			Assert::AreEqual(string("d1h5"), FindMove(PositionMode::FenWindow, LongMateInOne));
		}

		TEST_METHOD(DebugDump_ShowsThePositionOfTheMoves_Test)
		{
			ChessEnginePlayer player(EngineLevel{ 5 }, PositionMode::FenWindow);
			auto dump = player.DebugDump(Play(MateInOne));
			player.KillEngine();
			Assert::IsTrue(dump.find("side to move: white, plies played: 4") != string::npos, L"side and plies");
			Assert::IsTrue(dump.find("moves: e2e4 f7f6 d2d4 g7g5") != string::npos, L"moves");
		}
	};

	TEST_CLASS(EngineWorkerTests)
	{
	public:
		TEST_METHOD(FindMove_AnswersWithTheRequestNumber_Test)
		{
			EngineWorker worker(EngineLevel{ 5 });
			int request = -1;
			QString move;
			QObject::connect(&worker, &EngineWorker::moveFound, [&](int r, QString m) { request = r; move = m; });
			worker.findMove(7, Play(MateInOne));
			worker.killEngine();
			Assert::AreEqual(7, request);
			Assert::AreEqual(string("d1h5"), move.toStdString());
		}

		TEST_METHOD(FindMove_UnresolvedPromotion_IsAnError_Test)
		{
			// b7xa8 waits for the piece: the engine must not get "b7a8".
			auto history = Play({ "a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6b7", "b8c6", "b7a8" });
			EngineWorker worker(EngineLevel{ 5 });
			int found = 0, request = -1;
			QString details;
			QObject::connect(&worker, &EngineWorker::moveFound, [&](int, QString) { ++found; });
			QObject::connect(&worker, &EngineWorker::moveError, [&](int r, QString, QString d) { request = r; details = d; });
			worker.findMove(3, history);
			worker.killEngine();
			Assert::AreEqual(0, found);
			Assert::AreEqual(3, request);
			Assert::IsTrue(details.contains("b7a8"), L"the dump lists the moves");
		}

		// The worker on its own thread, as ChessConnector runs it: killEngine() from another thread
		// (the GUI thread's stop) must end a blocking search and let the thread exit (BL-15).
		TEST_METHOD(Kill_StopsAThinkingWorker_FromAnotherThread_Test)
		{
			auto* worker = new EngineWorker(EngineLevel{ 5 }); // movetime 2500 ms
			auto* thread = new QThread();
			worker->moveToThread(thread);
			QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);
			thread->start();

			std::atomic<bool> started{ false };
			const auto history = Play(MateInOne);
			QMetaObject::invokeMethod(worker, [worker, &started, history] {
				started = true;
				worker->findMove(1, history);
			}, Qt::QueuedConnection);

			// Let the engine start and the search begin, then stop it like the GUI thread does.
			for (int i = 0; i < 300 && !started.load(); ++i)
				QThread::msleep(10);
			QThread::msleep(300);
			QElapsedTimer timer;
			timer.start();
			worker->killEngine();
			thread->quit();

			Assert::IsTrue(thread->wait(3000), L"the engine thread stopped");
			Assert::IsTrue(timer.elapsed() < 2500, L"killEngine unblocked the search");
			delete thread;
		}
	};
}
