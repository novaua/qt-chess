#include "stdafx.h"
#include <CppUnitTest.h>
#include <set>
#include "../ChessCore/Game.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

namespace ChessTests {

// Plays the moves (UCI), then takes the last one back: the position must be the one before it
// (FEN: pieces, side to move, castling, en passant), every square that changed back must be
// reported to the board listeners, and playing the move again gives the same position as before.
static void CheckUndoOfLastMove(const std::vector<std::string>& moves)
{
    Game game;
    game.Restart();
    for (size_t i = 0; i + 1 < moves.size(); ++i)
        game.DoMove(Move::Parse(moves[i]));
    const auto fenBefore = game.MakeFen();

    game.DoMove(Move::Parse(moves.back()));
    const auto fenAfter = game.MakeFen();
    const Board boardAfter = game.GetCurrentBoard();

    std::set<int> notified;
    game.RegisterBoardChanged([&notified](int index, const Piece&) { notified.insert(index); });
    game.UndoMove();

    Assert::AreEqual(fenBefore, game.MakeFen(), L"position after the takeback");
    for (int i = 0; i < BpMax; ++i)
        if (!(boardAfter.At(i) == game.GetPieceAt(i)))
            Assert::IsTrue(notified.count(i) == 1, (L"square not reported: " + std::to_wstring(i)).c_str());

    game.DoMove(Move::Parse(moves.back()));
    Assert::AreEqual(fenAfter, game.MakeFen(), L"position after playing the move again");
}

TEST_CLASS(UndoTests) {
public:

    TEST_METHOD(QuietMove) {
        CheckUndoOfLastMove({ "e2e4" });
    }

    TEST_METHOD(Capture) {
        CheckUndoOfLastMove({ "e2e4", "d7d5", "e4d5" });
    }

    TEST_METHOD(CastlingWhiteKingside) {
        CheckUndoOfLastMove({ "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "e1g1" });
    }

    TEST_METHOD(CastlingWhiteQueenside) {
        CheckUndoOfLastMove({ "d2d4", "d7d5", "b1c3", "b8c6", "c1f4", "c8f5", "d1d2", "d8d7", "e1c1" });
    }

    TEST_METHOD(CastlingBlackKingside) {
        CheckUndoOfLastMove({ "e2e4", "e7e5", "g1f3", "g8f6", "f1c4", "f8c5", "b1c3", "e8g8" });
    }

    TEST_METHOD(CastlingBlackQueenside) {
        CheckUndoOfLastMove({ "d2d4", "d7d5", "c2c4", "b8c6", "b1c3", "c8f5", "g1f3", "d8d7", "e2e3", "e8c8" });
    }

    TEST_METHOD(EnPassantWhite) {
        CheckUndoOfLastMove({ "e2e4", "a7a6", "e4e5", "d7d5", "e5d6" });
    }

    TEST_METHOD(EnPassantBlack) {
        CheckUndoOfLastMove({ "a2a3", "e7e5", "a3a4", "e5e4", "d2d4", "e4d3" });
    }

    // A pawn that could take en passant takes a piece normally instead.
    TEST_METHOD(PawnCaptureNextToADoubleStep) {
        CheckUndoOfLastMove({ "e2e4", "g8f6", "e4e5", "d7d5", "e5f6" });
    }

    TEST_METHOD(PromotionWhite) {
        CheckUndoOfLastMove({ "h2h4", "g7g5", "h4g5", "h7h6", "g5h6", "g8f6", "h6h7", "h8g8", "h7h8q" });
    }

    TEST_METHOD(PromotionWithCaptureWhite) {
        CheckUndoOfLastMove({ "h2h4", "g7g5", "h4g5", "h7h6", "g5h6", "g8f6", "h6h7", "f6g8", "h7g8n" });
    }

    TEST_METHOD(PromotionWithCaptureBlack) {
        CheckUndoOfLastMove({ "g2g4", "h7h5", "a2a3", "h5g4", "a3a4", "g4g3", "a4a5", "g3h2", "a5a6", "h2g1q" });
    }

    // The player's promotion: the pawn reaches the last rank first, then the chosen piece replaces it.
    TEST_METHOD(PromotionChosenByThePlayer) {
        Game game;
        game.Restart();
        for (auto m : { "h2h4", "g7g5", "h4g5", "h7h6", "g5h6", "g8f6", "h6h7", "h8g8" })
            game.DoMove(Move::Parse(m));
        const auto fenBefore = game.MakeFen();

        game.RegisterGameActionsListeners([](const EventBase& event) {
            if (event.GetType() == EtPawnPromotion) {
                const auto& promotion = (const PawnPromotionEvent&)event;
                promotion.OnPromoted({ (BoardPosition)promotion.GetIndex(), { QUEEN, PieceColors::Light } });
            }
        });
        game.DoMove(h7, h8);
        Assert::IsTrue(game.GetPieceAt(h8) == Piece{ QUEEN, PieceColors::Light });

        game.UndoMove();
        Assert::AreEqual(fenBefore, game.MakeFen());
    }

    // Takeback against the robot: the robot's reply and the player's castling.
    TEST_METHOD(TwoMovesBack) {
        Game game;
        game.Restart();
        for (auto m : { "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6" })
            game.DoMove(Move::Parse(m));
        const auto fenBefore = game.MakeFen();
        game.DoMove(Move::Parse("e1g1"));
        game.DoMove(Move::Parse("f6e4"));
        game.UndoMove();
        game.UndoMove();
        Assert::AreEqual(fenBefore, game.MakeFen());
    }
};
}
