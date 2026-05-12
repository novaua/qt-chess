#include "stdafx.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "ChessException.h"
#include "Move.h"
#include "Game.h"
#include "Serializer.h"
#include "LruCacheMap.hpp"

namespace fs = std::filesystem;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

namespace ChessTests
{
	TEST_CLASS(BasicTests)
	{
	public:

		TEST_METHOD(IsInCheckMate_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			auto historyPtr = std::make_shared<MovesHistory>();
			auto cachePtr = std::make_shared<BoardPositionsCache>();

			GameState state = { boardPtr, historyPtr, cachePtr };

			boardPtr->Initialize();

			boardPtr->DoMove({ f2, f3 });
			boardPtr->DoMove({ e7, e6 });

			boardPtr->DoMove({ g2, g4});
			boardPtr->DoMove({ d8, h4});

			GameChecks check(state);

			Assert().IsTrue(check.IsInCheck(PieceColors::Light));
			Assert().IsTrue(check.IsCheckMate(PieceColors::Light));
		}
	};
}
