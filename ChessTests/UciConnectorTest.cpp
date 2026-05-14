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

		TEST_METHOD_CLEANUP(teardownTests)
		{}

		TEST_METHOD(MainInit_IsGoood_Test)
		{
			auto conn = new UciConnector();

			conn->Init();

			Assert::IsTrue(conn->IsInitialized());
			Assert::IsTrue(conn->GetOption("id").find("Stockfish") != string::npos);
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
			_connector->SetDifficulty(0);
			Assert::AreEqual(string("0"), _connector->GetOption("Skill Level"),
				L"Difficulty 0 should be reflected by GetOption");

			_connector->SetDifficulty(20);
			Assert::AreEqual(string("20"), _connector->GetOption("Skill Level"),
				L"Difficulty 20 should be reflected by GetOption");

			_connector->SetDifficulty(10);
			Assert::AreEqual(string("10"), _connector->GetOption("Skill Level"),
				L"Difficulty 10 should be reflected by GetOption");
		}

		TEST_METHOD(MakeMove_AfterDifficultySet_Test)
		{
			_connector->SetDifficulty(1);
			Assert::AreEqual(string("1"), _connector->GetOption("Skill Level"));

			auto response = _connector->GetEngineMove({ {}, { "e2e4", "e7e5", "b1c3" } }, chrono::milliseconds(2000));

			Assert::IsFalse(response.BestMove.empty(),
				L"Engine must return a move at low difficulty");
		}

		TEST_METHOD(Checkmate_Detection_Test)
		{
			_connector->SetDifficulty(1);
			Assert::AreEqual(string("1"), _connector->GetOption("Skill Level"));

			auto response = _connector->GetEngineMove({ {}, { "f2f3", "e7e6", "g2g4", "d8h4"} }, chrono::milliseconds(500));

			Assert::AreEqual(string("(none)"), response.BestMove,
				L"Engine must return a move at low difficulty");
		}

		TEST_METHOD(PawnPromotion_Detection_Test)
		{
			_connector->SetDifficulty(1);
			Assert::AreEqual(string("1"), _connector->GetOption("Skill Level"));

			auto moves1 = std::vector<std::string>{
				 "e2f1", "h2h3",
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