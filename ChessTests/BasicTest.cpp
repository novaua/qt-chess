#include "stdafx.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "ChessException.h"
#include "Move.h"
#include "Game.h"
#include "Serializer.h"

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
			auto boardPtr = new Board();

			boardPtr->DoMove({ a2, a4, false });
			boardPtr->DoMove({ e7, e3, false }, true);

			int figs[] = { a1, b1, c1, g1, e2, a2, b2, f2 };
			int movesForFigs[] = { 2, 2, 0, 2, 0, 0, 2, 3 };
			int isCapturing[] = { 0, 0, 0, 0, 0, 0, 0, 1 };

			auto count = 0;
			for (auto a : figs)
			{
				auto wasCapturing = 0;
				auto moves = MoveGeneration::GenerateMoves(*boardPtr, (BoardPosition)a, LIGHT);

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

		TEST_METHOD(HistoryMoveSerialization_Works)
		{
			HistoryMove hm;

			hm.From = { a1, Piece{ KING, LIGHT } };
			hm.To = { a1, Piece{ QUEEN, LIGHT } };

			PositionPiece from = hm.From;
			Assert().AreEqual<int>(from.Piece.Type, KING);

		}

		TEST_METHOD(MovesSerialization_Works_Test)
		{
			auto tempPath = tr2::sys::path(getenv("TEMP"));
			tempPath /= "tempFile";

			Move moves[] = {
				{ a1, f5 },
				{ d3, f4, true },
				{ c2, c4, true },
			};

			int movesCount = sizeof(moves) / sizeof(*moves);

			{
				// writing 	
				std::ofstream outFileStream(tempPath, ios::out | ios::binary);

				outFileStream.write((char*)&movesCount, sizeof(int));

				for (auto move : moves)
				{
					BinarySerializer::Serialize(move, outFileStream);
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
					loadedMoves.push_back(BinarySerializer::Deserialize<Move>(fs));
				}
			}

			tr2::sys::remove_filename(tr2::sys::path(tempPath));

			Assert::AreEqual(movesCount, (int)loadedMoves.size());
			auto id = 0;
			for (auto move : loadedMoves)
			{
				Assert::AreEqual<int>(moves[id].From, move.From);
				Assert::AreEqual<int>(moves[id].To, move.To);
				++id;
			}
		}

		TEST_METHOD(MoveValidation_Works_Test)
		{
			auto boardPtr = std::make_unique<Board>();
			boardPtr->DoMove({ e2, e4 });

			Assert::ExpectException<ChessException>([&boardPtr]()
			{
				boardPtr->DoMove({ b1, b8 });
			});
		}

		TEST_METHOD(BasicGame_Works_Test)
		{
			auto game = std::make_unique<Game>();

			Assert::IsTrue(game->IsWhiteMove());
			Assert::AreEqual(0, game->GetMoveCount());

			std::vector <BoardPosition> observedMoves;
			game->RegisterBoardChanged([&observedMoves](int index, const Piece &piece)
			{
				observedMoves.push_back(BoardPosition(index));
			});

			game->DoMove(e2, e4);

			Assert::AreEqual<int>(observedMoves[0], e2);
			Assert::AreEqual<int>(observedMoves[1], e4);

			Assert::IsFalse(game->IsWhiteMove());
			game->DoMove(e7, e5);

			Assert::IsTrue(game->IsWhiteMove());

			Assert::AreEqual(4u, observedMoves.size());

			auto gameRecord = game->GetGameRecord();
			Assert::AreEqual(2u, gameRecord.size());
		}
	};
}
