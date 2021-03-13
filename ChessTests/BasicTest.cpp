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

		TEST_METHOD(MainInit_IsGoood_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			auto historyPtr = std::make_shared<MovesHistory>();

			GameState state = { boardPtr, historyPtr, {} };

			boardPtr->Initialize();

			boardPtr->DoMove({ a2, a4, false });
			boardPtr->DoMove({ e7, e3, false });

			int figs[] = { a1, b1, c1, g1, e2, a2, b2, f2 };
			int movesForFigs[] = { 2, 2, 0, 2, 0, 0, 2, 3 };
			int isCapturing[] = { 0, 0, 0, 0, 0, 0, 0, 1 };

			auto count = 0;
			for (auto a : figs)
			{
				auto wasCapturing = 0;
				auto moves = MoveGeneration::GenerateMoves(state, (BoardPosition)a, LIGHT);

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
			auto tempPath = fs::path(std::string(getenv("TEMP")));
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
				fs.read((char*)&count, sizeof(int));

				for (auto i = 0; i < count; ++i)
				{
					loadedMoves.push_back(BinarySerializer::Deserialize<Move>(fs));
				}
			}

			fs::remove(fs::path(tempPath));

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
			auto boardPtr = std::make_shared<Board>();
			auto historyPtr = std::make_shared<MovesHistory>();

			GameState state = { boardPtr, historyPtr, {} };

			boardPtr->Initialize();

			MoveGeneration::Validate(state, Move{ e2, e4 }, LIGHT);
			boardPtr->DoMove({ e2, e4 });

			Assert::ExpectException<ChessException>([&]()
				{
					MoveGeneration::Validate(state, Move{ b1, b8 }, LIGHT);
					boardPtr->DoMove({ b1, b8 });
				});
		}

		TEST_METHOD(BasicGame_Works_Test)
		{
			auto game = std::make_unique<Game>();
			game->Restart();
			Assert::IsTrue(game->IsWhiteMove());
			Assert::AreEqual(0, game->GetMoveCount());

			std::vector <BoardPosition> observedMoves;
			game->RegisterBoardChanged([&observedMoves](int index, const Piece& piece)
				{
					observedMoves.push_back(BoardPosition(index));
				});

			game->DoMove(e2, e4);

			Assert::AreEqual<int>(observedMoves[0], e2);
			Assert::AreEqual<int>(observedMoves[1], e4);

			Assert::IsFalse(game->IsWhiteMove());
			game->DoMove(e7, e5);

			Assert::IsTrue(game->IsWhiteMove());

			Assert::AreEqual<size_t>(4u, observedMoves.size());

			auto gameRecord = game->GetGameRecord();
			Assert::AreEqual<size_t>(2u, gameRecord.size());
		}

		TEST_METHOD(LoadSave_Works_Test)
		{
			auto game = std::make_shared<Game>();
			game->Restart();

			game->DoMove(e2, e4);
			game->DoMove(e7, e5);

			auto tempPath = fs::path(std::string(getenv("TEMP")));
			tempPath /= "tempFile1";

			game->Save(tempPath.generic_string());
			game->Load(tempPath.generic_string());

			fs::remove(fs::path(tempPath));

			auto player = game->MakePlayer();
			Assert::IsTrue((bool)player);
		}

		TEST_METHOD(PositionPiecedHash_Test)
		{
			map<int, PositionPiece> hashPp;

			for (int i = 0; i < 64; ++i)
			{
				for (int j = 1; j < EPC_MAX; ++j)
				{
					for (int k = 1; k < 3; ++k)
					{
						//if (j == 0 && k > 0 || k == 0 && j > 0)
						//	continue;

						Piece p = { (EPieceTypes)j, (EPieceColors)k };
						PositionPiece pp = { (BoardPosition)i, p };

						if (hashPp.find(pp.GetHashCode()) == hashPp.end())
						{
							hashPp[pp.GetHashCode()] = { (BoardPosition)i, p };
						}
						else
						{
							auto fp = hashPp[p.GetHashCode()];

							Assert::AreEqual<int>(p.Type, fp.Piece.Type);
							Assert::AreEqual<int>(p.Color, fp.Piece.Color);
							Assert::AreEqual<int>(pp.Position, fp.Position);
						}
					}
				}
			}

			Assert::AreEqual<size_t >((2u * 6) * 64, hashPp.size());
		}

		TEST_METHOD(BoardHash_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			boardPtr->Initialize();
			auto hk1 = boardPtr->GetHashCode();

			boardPtr->DoMove({ e2, e4 });
			auto hk2 = boardPtr->GetHashCode();

			PositionPiece pE2 = { e2, { PAWN, LIGHT } };
			PositionPiece pE4 = { e4, { PAWN, LIGHT } };
			PositionPiece eE4 = { e4, { EMPTY, CEMPTY } };
			PositionPiece eE2 = { e2, { EMPTY, CEMPTY } };

			// incremental has allows moves reversion and actually works
			auto hk1a = hk2 ^ pE4.GetHashCode() ^ eE4.GetHashCode()
				^ eE2.GetHashCode() ^ pE2.GetHashCode();


			// Checking Zobrist Hashing incremental property
			Assert::AreEqual(hk1, hk1a);

			Assert::AreNotEqual(hk1, hk2);
			std::cout << "e2e4 hash " << boardPtr->GetHashCode();

			boardPtr->DoMove({ e4, e2 });

			auto hk3 = boardPtr->GetHashCode();

			Assert::AreEqual(hk1, hk3);
		}

		TEST_METHOD(BoardHash_1_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			boardPtr->Initialize();
			std::vector<size_t> hashCodes;
			hashCodes.push_back(boardPtr->GetHashCode());

			boardPtr->DoMove({ e2, e4 });
			hashCodes.push_back(boardPtr->GetHashCode());

			boardPtr->DoMove({ e7, e5 });
			hashCodes.push_back(boardPtr->GetHashCode());

			boardPtr->DoMove({ f1, b5 });
			hashCodes.push_back(boardPtr->GetHashCode());

			std::sort(hashCodes.begin(), hashCodes.end());
			size_t prev = 0u;
			for (auto hk : hashCodes)
			{
				Assert::AreNotEqual< size_t >(hk, prev);
				prev = hk;
			}
		}

		TEST_METHOD(BoardUndo_Test)
		{
			auto boardPtr = std::make_shared<Board>();
			boardPtr->Initialize();
			auto initBoardCode = boardPtr->GetHashCode();

			boardPtr->DoMove({ e2, e4 });

			Assert::AreNotEqual(initBoardCode, boardPtr->GetHashCode());
			boardPtr->UndoLastMove();
			Assert::AreEqual(initBoardCode, boardPtr->GetHashCode());
		}

		TEST_METHOD(LruCacheMap_Test)
		{
			int Count = 23;
			int MaxCache = 5;
			LruCacheMap<int, int> cache(MaxCache);

			for (int i = 0; i < Count; ++i)
			{
				cache.AddOrUpdate(i, Count - i);
				Assert::IsTrue(cache.Contains(i));
				Assert::AreEqual(Count - i, cache.find(i)->second);
			}

			for (int i = Count - 1; i >= 0; --i)
			{
				if (Count - i <= MaxCache)
				{
					Assert::IsTrue(cache.Contains(i));
				}
				else {
					Assert::IsFalse(cache.Contains(i));
				}
			}
		}

		TEST_METHOD(MoveToString_Test)
		{
			Move moves[] =
			{ { e2, e4, false, { QUEEN, LIGHT } },
			{ e2, e4, true, {} } };

			for(const auto &move: moves)
			{
				auto strMove = move.ToString();

				auto moveR = Move::Parse(strMove);

				Assert::AreEqual<int>(move.From, moveR.From);
				Assert::AreEqual<int>(move.To, moveR.To);
				Assert::AreEqual<int>(move.Capturing, moveR.Capturing);
				Assert::AreEqual<int>(move.PromotedTo.Type, moveR.PromotedTo.Type);
			}
		}
	};
}
