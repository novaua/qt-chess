#include "stdafx.h"
#include <CppUnitTest.h>
#include <algorithm>
#include <sstream>
#include "../FakeUci/fakeengine.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ChessTests {

// FakeUci (BL-05 step 3): the UI tests' stand-in engine. Its moves must be legal and the same every
// time: the scripted game's move when legal, else the first legal move by its UCI text.
TEST_CLASS(FakeEngineTests)
{
    static bool Has(const std::vector<std::string>& moves, const std::string& move)
    {
        return std::find(moves.begin(), moves.end(), move) != moves.end();
    }

public:
    TEST_METHOD(StartPosition_TwentyMoves_FirstByText)
    {
        FakeEngine engine({}, 0);
        const auto moves = engine.LegalMoves();
        Assert::AreEqual(size_t(20), moves.size());
        Assert::AreEqual(std::string("a2a3"), engine.ChooseMove());
    }

    TEST_METHOD(Script_PlaysTheGamesMoveAtItsPly)
    {
        FakeEngine engine({ "e2e4", "e7e5", "g1f3", "b8c6" }, 0);
        Assert::IsTrue(engine.Play("e2e4"));
        Assert::AreEqual(std::string("e7e5"), engine.ChooseMove());
        Assert::IsTrue(engine.Play("e7e5"));
        Assert::IsTrue(engine.Play("g1f3"));
        Assert::AreEqual(3, engine.Ply());
        Assert::AreEqual(std::string("b8c6"), engine.ChooseMove());
    }

    TEST_METHOD(Script_IllegalThere_FirstLegalInstead)
    {
        FakeEngine engine({ "e2e4", "e2e4" }, 0); // the second isn't Black's move
        Assert::IsTrue(engine.Play("e2e4"));
        Assert::AreEqual(std::string("a7a5"), engine.ChooseMove());
    }

    TEST_METHOD(IllegalMove_NotPlayed)
    {
        FakeEngine engine({}, 0);
        Assert::IsFalse(engine.Play("e2e5"));
        Assert::IsFalse(engine.Play("e7e5")); // White to move
        Assert::AreEqual(0, engine.Ply());
    }

    TEST_METHOD(Fen_SideToMoveAndPly)
    {
        FakeEngine engine({}, 0);
        Assert::IsTrue(engine.SetFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"));
        Assert::AreEqual(1, engine.Ply());
        const auto moves = engine.LegalMoves();
        Assert::IsTrue(Has(moves, "e7e5"));
        Assert::IsFalse(Has(moves, "d2d4"));
        Assert::IsTrue(engine.SetFen("4k3/8/8/8/8/8/8/4K3 w - - 0 31"));
        Assert::AreEqual(60, engine.Ply());
    }

    TEST_METHOD(Fen_CastlingOnlyWhereTheFenAllows)
    {
        FakeEngine engine({}, 0);
        Assert::IsTrue(engine.SetFen("r3k2r/8/8/8/8/8/8/R3K2R w Kq - 0 1"));
        Assert::IsTrue(Has(engine.LegalMoves(), "e1g1"));
        Assert::IsFalse(Has(engine.LegalMoves(), "e1c1"));
        Assert::IsTrue(engine.Play("e1g1"));
        Assert::IsTrue(Has(engine.LegalMoves(), "e8c8"));
        Assert::IsFalse(Has(engine.LegalMoves(), "e8g8"));
    }

    TEST_METHOD(Fen_EnPassantFromItsSquare)
    {
        FakeEngine engine({}, 0);
        Assert::IsTrue(engine.SetFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2"));
        Assert::IsTrue(Has(engine.LegalMoves(), "e5d6"));
        Assert::IsTrue(engine.SetFen("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 2"));
        Assert::IsFalse(Has(engine.LegalMoves(), "e5d6"));
    }

    TEST_METHOD(Promotion_NamesItsPiece)
    {
        FakeEngine engine({}, 0);
        Assert::IsTrue(engine.SetFen("8/P7/8/8/8/8/8/k6K w - - 0 1"));
        Assert::IsTrue(Has(engine.LegalMoves(), "a7a8q"));
        Assert::IsFalse(engine.IsLegal("a7a8"));  // a promotion without its piece
        Assert::IsTrue(engine.IsLegal("a7a8n"));  // the player's own choice
        Assert::IsTrue(engine.Play("a7a8n"));
    }

    TEST_METHOD(Mated_NoMove)
    {
        FakeEngine engine({}, 0);
        for (const char* move : { "f2f3", "e7e5", "g2g4", "d8h4" })
            Assert::IsTrue(engine.Play(move));
        Assert::IsTrue(engine.LegalMoves().empty());
        Assert::AreEqual(std::string(), engine.ChooseMove());
    }

    TEST_METHOD(Protocol_AsTheAppTalksToIt)
    {
        FakeEngine engine({ "e2e4", "c7c5" }, 0);
        std::istringstream in("uci\nisready\nsetoption name MultiPV value 3\nucinewgame\n"
                              "position startpos moves e2e4\ngo depth 5\nquit\nisready\n");
        std::ostringstream out;
        engine.Run(in, out);
        const std::string text = out.str();
        Assert::IsTrue(text.find("option name UCI_Elo") != std::string::npos);
        Assert::IsTrue(text.find("uciok") != std::string::npos);
        Assert::IsTrue(text.find("info depth 1 multipv 1 score cp 0 pv c7c5") != std::string::npos);
        Assert::IsTrue(text.find("bestmove c7c5") != std::string::npos);
        Assert::AreEqual(text.find("readyok"), text.rfind("readyok")); // one: nothing is read after quit
    }
};

}
