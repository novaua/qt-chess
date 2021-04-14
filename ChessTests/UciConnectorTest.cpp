#include "stdafx.h"
#include <CppUnitTest.h>
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
			Assert::IsTrue(conn->GetOption("id").find("St") != string::npos);
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