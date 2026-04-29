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
		{
		}

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
			auto response = _connector->GetEngineMove({ { "e2e4",  "e7e5" , "b1c3" } }, chrono::seconds(1));

			Assert::IsFalse(response.BestMove.empty());
			Assert::IsFalse(response.Ponder.empty());
		}
	};
}