#include "stdafx.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "ChessMain.h"
#include "ChessException.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

namespace ChessTests
{
	TEST_CLASS(BasicTests)
	{
	public:

		TEST_METHOD(MainInit_IsGoood_Test)
		{
			auto chessMain = new ChessMain();
			auto boardPtr = new Board(true);
			boardPtr->DoMove({ a2, a4, false, {} });

			boardPtr->DoMove({ e7, e3, false, {} }, true);

			int figs[] = { a1, b1, c1, g1, e2, a2, b2, f2 };
			int movesForFigs[] = { 2, 2, 0, 2, 0, 0, 2, 3 };
			bool isCapturing[] = { 0, 0, 0, 0, 0, 0, 0, 1 };

			auto count = 0;
			for (auto a : figs)
			{
				auto wasCapturing = 0;
				auto moves = MoveGeneration::GenerateMoves(*boardPtr, a, LIGHT);

				Assert().IsTrue(moves.size() == movesForFigs[count]);

				cout << "Moves for " << a << endl;
				for (auto move : moves)
				{
					cout << "Move to " << move.To << (move.Capturing ? " capturing" : " free") << endl;
					wasCapturing += move.Capturing ? 1 : 0;
				}

				Assert().IsTrue(isCapturing[count] == wasCapturing);

				++count;
			}
		}

		TEST_METHOD(MovesSerialization_Works_Test)
		{
			auto tempPath = tr2::sys::path(getenv("TEMP"));
			tempPath /= "tempFile";

			Move moves[] =
			{ { 0, 33 },
			{ 1, 44, true, { (EPieceTypes)2, (EPieceColors)0 } },
			{ 2, 55, true, { (EPieceTypes)2, (EPieceColors)1 } },
			};

			int movesCount = sizeof(moves) / sizeof(*moves);

			{
				// writing 	
				std::ofstream outFileStream(tempPath, ios::out | ios::binary);

				outFileStream.write((char*)&movesCount, sizeof(int));

				for (auto move : moves)
				{
					move.Serialize(outFileStream);
				}
			}

			std::list<Move> loadedMoves;
			{
				// reading 	
				std::ifstream fs(tempPath, ios::in | ios::binary);
				int count = 0;
				fs.read((char *)&count, sizeof(int));

				for (auto i = 0; i < count; ++i)
				{
					loadedMoves.push_back(Move::Deserialize(fs));
				}
			}

			tr2::sys::remove_filename(tr2::sys::path(tempPath));

			Assert::AreEqual(movesCount, (int)loadedMoves.size());
			auto id = 0;
			for (auto move : loadedMoves)
			{
				Assert::AreEqual(moves[id].From, move.From);
				Assert::AreEqual(moves[id].To, move.To);
				++id;
			}
		}

		TEST_METHOD(MoveValidation_Works_Test)
		{
			auto boardPtr = std::unique_ptr<Board>(new Board(true));
			boardPtr->DoMove({ e2, e4 });

			Assert::ExpectException<ChessException>([&boardPtr]()
			{
				boardPtr->DoMove({ b1, b8 });
			});
		}
	};
}
