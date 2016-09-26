#include "stdafx.h"
#include "pstream.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "UciConnector.h"
#include "Utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

namespace ChessTests
{
    TEST_CLASS(IntegrationTests)
    {
        std::string _exePath = "D:\\tools\\stockfish-7-win\\Windows\\stockfish\ 7\ 32bit.exe";

    public:
        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            Logger::WriteMessage("In Class Initialize");
        }

        TEST_CLASS_CLEANUP(ClassCleanup)
        {
            Logger::WriteMessage("In Class Cleanup");
        }

        TEST_METHOD(EnginePingBasic_Test)
        {
            redi::pstream engine(_exePath);

            std::string answer;
            auto ok = false;
            for (int i = 0; i < 5 && !ok; ++i)
            {
                if (std::getline(engine, answer))
                {
                    std::cout << "answer:" << answer;
                    ok = begins_with(answer, std::string("readyok"));
                    if (!ok)
                    {
                        engine << "isready" << endl;
                    }
                }
                else
                {
                    std::cout << "No answer!";
                    break;
                }
            }
        }

        TEST_METHOD(UciConnector_Init_Test)
        {
            redi::pstream engine(_exePath);
            UciConnector cnt(engine, engine);

            auto optionsMap = cnt.Init();
            Assert::IsTrue(optionsMap.size() > 1);
        }

        TEST_METHOD(UciConnector_Move_Test)
        {
            redi::pstream engine(_exePath);
            UciConnector cnt(engine, engine);

            cnt.Init();

            Assert::IsTrue(cnt.IsReady());
            cnt.PositionMoves({ e2, e4 });
            auto moves = cnt.Go();


            Logger::WriteMessage((std::string("Move ") + moves.first.ToString()).c_str());
            Logger::WriteMessage((std::string("Move ") + moves.second.ToString()).c_str());

            Assert::AreNotEqual<int>(moves.first.From, Move::Empty.From);
            Assert::AreNotEqual<int>(moves.second.To, Move::Empty.To);
        }
    };
}