#include "stdafx.h"
#include <CppUnitTest.h>
#include <algorithm>
#include "../ChessCore/Game.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

namespace ChessTests {

// Game::GetPremoveTargets (LI-05): where a piece may be premoved while the other side is to move.
TEST_CLASS(PremoveTests)
{
    static std::shared_ptr<Game> Played(std::initializer_list<const char*> moves)
    {
        auto game = std::make_shared<Game>();
        game->Restart();
        for (const char* move : moves)
            game->DoMove(Move::Parse(move));
        return game;
    }

    static std::vector<BoardPosition> Sorted(std::vector<BoardPosition> squares)
    {
        std::sort(squares.begin(), squares.end());
        return squares;
    }

    static void AssertTargets(std::vector<BoardPosition> expected, const std::vector<BoardPosition>& actual)
    {
        Assert::IsTrue(Sorted(expected) == Sorted(actual));
    }

    static bool Has(const std::vector<BoardPosition>& squares, BoardPosition square)
    {
        return std::find(squares.begin(), squares.end(), square) != squares.end();
    }

public:
    TEST_METHOD(Knight_AllJumps_AlsoOntoOwnPieces)
    {
        auto game = Played({});
        AssertTargets({ a3, c3, d2 }, game->GetPremoveTargets(b1));
    }

    // Forward, two from the start, and both captures even with nothing to take yet.
    TEST_METHOD(Pawn_StepsAndBothCaptures)
    {
        auto game = Played({});
        AssertTargets({ e3, e4, d3, f3 }, game->GetPremoveTargets(e2));
        AssertTargets({ e6, e5, d6, f6 }, game->GetPremoveTargets(e7));
    }

    // Our own piece in front stays there (or is replaced by theirs): no step. Theirs may move away.
    TEST_METHOD(Pawn_BlockedByOwnPiece_NotByTheirs)
    {
        auto game = Played({ "e2e4", "e7e5", "g1f3", "d7d5", "f3e5" });
        Assert::IsTrue(Has(game->GetPremoveTargets(e4), e5) == false); // our knight on e5
        auto black = game->GetPremoveTargets(d5);
        Assert::IsTrue(Has(black, d4) && Has(black, e4) && Has(black, c4));

        auto blocked = Played({ "d2d4", "d7d5" }); // pawn against pawn: each may step if the other goes
        Assert::IsTrue(Has(blocked->GetPremoveTargets(d5), d4));
        Assert::IsTrue(Has(blocked->GetPremoveTargets(d4), d5));
    }

    // In the start position the rook only reaches the two pieces next to it.
    TEST_METHOD(Rook_LineStopsAtOwnPiece_Inclusive)
    {
        auto game = Played({});
        AssertTargets({ a2, b1 }, game->GetPremoveTargets(a1));
    }

    // A line passes one of their pieces (it may move away), not two.
    TEST_METHOD(Bishop_LinePassesOneOfTheirs_NotTwo)
    {
        auto game = Played({ "b2b3", "e7e5", "c1b2", "f7f6" });
        AssertTargets({ a1, a3, c1, c3, d4, e5, f6 }, game->GetPremoveTargets(b2));
    }

    TEST_METHOD(Queen_NeverOntoOwnKing)
    {
        auto game = Played({});
        AssertTargets({ c1, c2, d2, e2 }, game->GetPremoveTargets(d1));
    }

    TEST_METHOD(King_CastlesWhileKingAndRookHaventMoved)
    {
        auto game = Played({ "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6" });
        auto king = game->GetPremoveTargets(e1);
        Assert::IsTrue(Has(king, g1));  // f1 and g1 are free
        Assert::IsFalse(Has(king, c1)); // our queen on d1
        Assert::IsTrue(Has(king, e2) && Has(king, f1) && Has(king, d1));

        game->DoMove(Move::Parse("e1e2"));
        game->DoMove(Move::Parse("f6g4"));
        game->DoMove(Move::Parse("e2e1"));
        Assert::IsFalse(Has(game->GetPremoveTargets(e1), g1)); // the king has moved
    }

    // A premoved pawn to the last rank becomes a queen (ChessConnector asks this).
    TEST_METHOD(IsPromotion_PawnToLastRank)
    {
        auto game = Played({ "a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6a7", "b7c6" });
        Assert::IsTrue(game->IsPromotion(a7, b8));
        Assert::IsTrue(game->IsPromotion(a7, a8));
        Assert::IsFalse(game->IsPromotion(e2, e4));
        Assert::IsFalse(game->IsPromotion(b1, a3));
        Assert::IsFalse(game->IsPromotion(-1, a8));
    }

    TEST_METHOD(EmptySquare_NoTargets)
    {
        auto game = Played({});
        Assert::IsTrue(game->GetPremoveTargets(e4).empty());
        Assert::IsTrue(game->GetPremoveTargets(-1).empty());
        Assert::IsTrue(game->GetPremoveTargets(64).empty());
    }
};

}
