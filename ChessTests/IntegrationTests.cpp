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

        TEST_METHOD(UciConnectorBasic_Test)
        {
            redi::pstream engine(_exePath);
            UciConnector cnt(engine, engine);

            auto optionsMap = cnt.Init();
            Assert::IsTrue(optionsMap.size() > 1);
        }
    };
}