#include "stdafx.h"
#include "pstream.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "IcuConnector.h"
#include "Utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Chess;
using namespace std;

namespace ChessTests
{
    TEST_CLASS(IntegrationTests)
    {
    public:

        TEST_METHOD(IcuConnectorBasic_Test)
        {
            auto exePath = "D:\\tools\\stockfish-7-win\\Windows\\stockfish\ 7\ 32bit.exe";

            redi::pstream engine(exePath);
           
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
                        engine << "isready"<<endl;
                    }
                }
                else 
                {
                    std::cout << "No answer!";
                    break;
                }
            }
        }
    };
}