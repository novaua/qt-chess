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

		TEST_METHOD(IsInCheckNotMate_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			auto historyPtr = std::make_shared<MovesHistory>();
			auto cachePtr = std::make_shared<BoardPositionsCache>();

			GameState state = { boardPtr, historyPtr, cachePtr };

			boardPtr->Initialize();
			auto moves = {
				  Move(e4, c3, true),Move(c2, c3),
				  Move(a6, b4), Move(e2, e3),
				  Move(d7, d5),Move(b1, d2),
				  Move(e8, g8),Move(e1, d1),
				  Move(c5, f2, true), Move(d1, c2),
				  Move(f6, e4),Move(c1, f4),
				  Move(b8, a6),Move(c3, c4),
				  Move(f8, c5, true), Move(d4, c5, true),
				  Move(c8, b7),Move(a2, a4),
				  Move(b7, b6),Move(b2, b3),
				  Move(c7, c5),Move(c2, c3),
				  Move(e7, e6),Move(g1, f3),
				  Move(g8, f6),Move(d2, d4)
			};

			// reverse moves to get the correct order
			for (auto it = std::rbegin(moves); it != std::rend(moves); ++it)
			{
				boardPtr->DoMove(*it);
			}

			GameChecks check(state);

			Assert().IsTrue(check.IsInCheck(PieceColors::Light));
			Assert().IsFalse(check.IsCheckMate(PieceColors::Light));
		}
	};
}
