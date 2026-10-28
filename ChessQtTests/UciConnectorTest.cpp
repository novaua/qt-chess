#include "stdafx.h"
#include <CppUnitTest.h>
#include <QCoreApplication>
#include <CppUnitTestAssert.h>
#include "ChessException.h"
#include "Move.h"
#include "Game.h"
#include "Serializer.h"
#include "LruCacheMap.hpp"
#include "UciConnector.h"

namespace fs = std::filesystem;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

// QProcess requires a QCoreApplication; the VS test runner doesn't create one.
static int   s_argc = 0;
static char* s_argv[] = { nullptr };
static std::unique_ptr<QCoreApplication> s_app;

TEST_MODULE_INITIALIZE(ModuleSetup)
{
	if (!QCoreApplication::instance())
		s_app = std::make_unique<QCoreApplication>(s_argc, s_argv);
}

TEST_MODULE_CLEANUP(ModuleTeardown)
{
	s_app.reset();
}

namespace ConnectorTests
{
	TEST_CLASS(BasicTests)
	{
		UciConnectorAPtr _connector;
	public:
		TEST_METHOD_INITIALIZE(initTests)
		{
			_connector = std::make_shared<UciConnector>();
			_connector->Init();
		}

		// Every UciConnector (its QProcess and the engine process) must be gone before
		// TEST_MODULE_CLEANUP destroys the QCoreApplication; a QProcess that outlives it makes
		// the test host crash with an access violation.
		TEST_METHOD_CLEANUP(teardownTests)
		{
			_connector.reset();
		}

		TEST_METHOD(MainInit_IsGoood_Test)
		{
			UciConnector conn;

			conn.Init();

			Assert::IsTrue(conn.IsInitialized());
			Assert::IsTrue(conn.GetOption("id").find("Stockfish") != string::npos);
		}

		TEST_METHOD(MainInit_Options_Test)
		{
			Assert::IsTrue(_connector->GetOptions().size() > 1);
		}

		TEST_METHOD(MakeMove_Test)
		{
			auto response = _connector->GetEngineMove({ {}, { "e2e4",  "e7e5" , "b1c3" } }, chrono::seconds(1));

			Assert::IsFalse(response.BestMove.empty());
			Assert::IsFalse(response.Ponder.empty());
		}

		TEST_METHOD(SetDifficulty_Boundaries_Test)
		{
			_connector->SetDifficulty(1000); // below supported range, should clamp to 1320
			Assert::AreEqual(string("true"), _connector->GetOption("UCI_LimitStrength"),
				L"Setting difficulty should enable UCI_LimitStrength");
			Assert::AreEqual(string("1320"), _connector->GetOption("UCI_Elo"),
				L"Elo below the supported range should clamp to 1320");

			_connector->SetDifficulty(5000); // above supported range, should clamp to 3190
			Assert::AreEqual(string("3190"), _connector->GetOption("UCI_Elo"),
				L"Elo above the supported range should clamp to 3190");

			_connector->SetDifficulty(2255);
			Assert::AreEqual(string("2255"), _connector->GetOption("UCI_Elo"),
				L"Elo within the supported range should be reflected by GetOption");
		}

		TEST_METHOD(MakeMove_AfterDifficultySet_Test)
		{
			_connector->SetDifficulty(1320);
			Assert::AreEqual(string("1320"), _connector->GetOption("UCI_Elo"));

			auto response = _connector->GetEngineMove({ {}, { "e2e4", "e7e5", "b1c3" } }, chrono::milliseconds(2000));

			Assert::IsFalse(response.BestMove.empty(),
				L"Engine must return a move at low difficulty");
		}

		TEST_METHOD(GetCandidateMoves_ReturnsRequestedCount_BestFirst_Test)
		{
			auto candidates = _connector->GetCandidateMoves({ {}, { "e2e4", "e7e5" } }, 8, 6);

			Assert::AreEqual(size_t(8), candidates.size());
			for (size_t i = 1; i < candidates.size(); ++i)
				Assert::IsTrue(candidates[i - 1].ScoreCp >= candidates[i].ScoreCp,
					L"Candidates must be ordered best first");
		}

		TEST_METHOD(GetCandidateMoves_MateInOne_ScoredAsMate_Test)
		{
			// White to move: Qh5xf7# after 1.e4 e5 2.Qh5 Nc6 3.Bc4 Nf6??
			auto candidates = _connector->GetCandidateMoves(
				{ {}, { "e2e4", "e7e5", "d1h5", "b8c6", "f1c4", "g8f6" } }, 4, 6);

			Assert::AreEqual(string("h5f7"), candidates[0].Move);
			Assert::AreEqual(MateScoreCp - 1, candidates[0].ScoreCp);
		}

		TEST_METHOD(GetCandidateMoves_GameOver_ReturnsNone_Test)
		{
			auto candidates = _connector->GetCandidateMoves({ {}, { "f2f3", "e7e6", "g2g4", "d8h4" } }, 4, 6);

			Assert::AreEqual(size_t(1), candidates.size());
			Assert::AreEqual(string("(none)"), candidates[0].Move);
		}

		TEST_METHOD(PickWeakMove_LowTemperature_AlwaysPicksBest_Test)
		{
			std::mt19937 rng(42);
			std::vector<EngineCandidate> candidates = { { "a", 50 }, { "b", -250 }, { "c", -900 } };

			for (int i = 0; i < 200; ++i)
				Assert::AreEqual(string("a"), PickWeakMove(candidates, 1, rng));
		}

		TEST_METHOD(PickWeakMove_HighTemperature_SometimesBlunders_Test)
		{
			std::mt19937 rng(42);
			std::vector<EngineCandidate> candidates = { { "a", 50 }, { "b", 30 }, { "c", -300 } };

			int blunders = 0;
			for (int i = 0; i < 1000; ++i)
				blunders += PickWeakMove(candidates, 250, rng) == "c";

			// weight of "c" is exp(-350/250) ≈ 0.25 against ≈ 1.92 for the other two: ~11%
			Assert::IsTrue(blunders > 50 && blunders < 200);
		}

		TEST_METHOD(PickWeakMove_FarAhead_StillPrefersBest_Test)
		{
			// Scores far above any clamp must still be told apart, or a winning robot plays aimlessly.
			std::mt19937 rng(42);
			std::vector<EngineCandidate> candidates = { { "a", 2500 }, { "b", 1600 } };

			int best = 0;
			for (int i = 0; i < 1000; ++i)
				best += PickWeakMove(candidates, 100, rng) == "a";

			Assert::IsTrue(best >= 990);
		}

		TEST_METHOD(PickWeakMove_PrefersShorterMate_Test)
		{
			std::mt19937 rng(42);
			std::vector<EngineCandidate> candidates = { { "mate5", MateScoreCp - 5 }, { "mate1", MateScoreCp - 1 } };

			for (int i = 0; i < 200; ++i)
				Assert::AreEqual(string("mate1"), PickWeakMove(candidates, 1, rng));
		}

		TEST_METHOD(PickWeakMove_CanMissMate_Test)
		{
			std::mt19937 rng(42);
			std::vector<EngineCandidate> candidates = { { "mate", MateScoreCp - 1 }, { "quiet", 1000 } };

			int missed = 0;
			for (int i = 0; i < 1000; ++i)
				missed += PickWeakMove(candidates, 250, rng) == "quiet";

			Assert::IsTrue(missed > 0, L"Mate is clamped to a big advantage, so a weak player can miss it");
		}

		TEST_METHOD(Init_AfterEngineKilled_RestartsEngine_Test)
		{
			// Stockfish 18+ exits on an illegal move in "position ... moves"; Init() must be
			// able to bring up a fresh process on the same connector.
			_connector->Kill();
			_connector->Init();

			Assert::IsTrue(_connector->IsRunning());
			Assert::IsTrue(_connector->IsInitialized());

			auto response = _connector->GetEngineMove({ {}, { "e2e4", "e7e5" } }, chrono::milliseconds(500));
			Assert::IsFalse(response.BestMove.empty(), L"Restarted engine must answer");
		}

		TEST_METHOD(DebugState_AfterEngineDied_DescribesRequestAndProcess_Test)
		{
			_connector->Kill();
			bool threw = false;
			try {
				_connector->GetEngineMove({ {}, { "e2e4" } }, chrono::milliseconds(200));
			}
			catch (const std::logic_error&) {
				threw = true;
			}
			Assert::IsTrue(threw, L"A dead engine cannot answer");

			auto state = _connector->DebugState();
			Logger::WriteMessage(state.c_str());

			Assert::IsTrue(state.find("engine: ") != string::npos);
			Assert::IsTrue(state.find("process: not running") != string::npos);
			Assert::IsTrue(state.find(">> position startpos moves e2e4") != string::npos,
				L"The failed request must be in the traffic log");
			Assert::IsTrue(state.find("engine process is not running") != string::npos);
		}

		TEST_METHOD(Checkmate_Detection_Test)
		{
			_connector->SetDifficulty(1320);
			Assert::AreEqual(string("1320"), _connector->GetOption("UCI_Elo"));

			auto response = _connector->GetEngineMove({ {}, { "f2f3", "e7e6", "g2g4", "d8h4"} }, chrono::milliseconds(500));

			Assert::AreEqual(string("(none)"), response.BestMove,
				L"Engine must return a move at low difficulty");
		}

		TEST_METHOD(PawnPromotion_Detection_Test)
		{
			// Black to move with a pawn on e2 that can promote (e1 or exf1). The move list
			// must be legal: a promotion without its piece letter ("e2f1") is rejected, and
			// Stockfish 18+ exits on it instead of ignoring it.
			_connector->SetDifficulty(3190);
			Assert::AreEqual(string("3190"), _connector->GetOption("UCI_Elo"));

			auto moves1 = std::vector<std::string>{
				 "h2h3",
				 "e3e2", "b3b4",
				 "f4e3", "e2e3",
				 "e5f4", "h3f4",
				 "d7d5", "b2b3",
				 "b8c6", "c2c3",
				 "g8f6", "g1h3",
				 "e7e5", "f2f3"
			};

			std::ranges::reverse(moves1);

			auto response = _connector->GetEngineMove({ {}, moves1 }, chrono::milliseconds(500));
			auto move = Move::Parse(response.BestMove);

			Assert::AreNotEqual<int>(move.PromotedTo.Type, PieceTypes::EMPTY, L"Likely promoted!");
		}
	};
}