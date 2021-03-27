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
	public:
		TEST_METHOD(MainInit_IsGoood_Test)
		{
			auto conn = new UciConnector();

			conn->Init();

			Assert::IsTrue(conn->IsInitialized());
		}
	};
}