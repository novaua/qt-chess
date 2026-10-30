#include "stdafx.h"
#include <CppUnitTest.h>
#include "../ChessCore/Game.h"
#include "../ChessCore/BoardDiff.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

namespace ChessTests {

static const Piece WhitePawn{ PAWN, PieceColors::Light };
static const Piece BlackPawn{ PAWN, PieceColors::Dark };

// The position after the moves (UCI) from the start.
static Board Position(const std::vector<std::string>& moves)
{
    Game game;
    game.Restart();
    for (const auto& m : moves)
        game.DoMove(Move::Parse(m));
    return game.GetCurrentBoard();
}

static std::vector<std::string> Plus(std::vector<std::string> moves, const std::string& move)
{
    moves.push_back(move);
    return moves;
}

static bool HasTravel(const BoardDiff& diff, BoardPosition from, BoardPosition to, Piece before, Piece after)
{
    for (const auto& t : diff.Moved())
        if (t.From() == from && t.To() == to && t.Before() == before && t.After() == after)
            return true;
    return false;
}

static bool Has(const std::vector<PositionPiece>& pieces, BoardPosition position, Piece piece)
{
    for (const auto& p : pieces)
        if (p.Position == position && p.Piece == piece)
            return true;
    return false;
}

TEST_CLASS(BoardDiffTests) {
public:

    TEST_METHOD(SamePositionNothingChanged) {
        const auto board = Position({ "e2e4", "e7e5" });
        Assert::IsTrue(BoardDiff(board, board).IsEmpty());
    }

    TEST_METHOD(QuietMoveAndItsTakeback) {
        const auto before = Position({});
        const auto after = Position({ "g1f3" });
        const Piece knight{ KNIGHT, PieceColors::Light };

        auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, g1, f3, knight, knight));

        diff = BoardDiff(after, before);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, f3, g1, knight, knight));
    }

    TEST_METHOD(CaptureAndItsTakeback) {
        const std::vector<std::string> moves = { "e2e4", "d7d5" };
        const auto before = Position(moves);
        const auto after = Position(Plus(moves, "e4d5"));

        auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, e4, d5, WhitePawn, WhitePawn));
        Assert::IsTrue(Has(diff.Removed(), d5, BlackPawn));

        diff = BoardDiff(after, before);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, d5, e4, WhitePawn, WhitePawn));
        Assert::IsTrue(Has(diff.Added(), d5, BlackPawn));
    }

    // King and rook both travel, both ways, for every castling.
    TEST_METHOD(Castling) {
        struct Case { std::vector<std::string> moves; std::string castling; BoardPosition kingTo, rookFrom, rookTo; PieceColors color; };
        const std::vector<Case> cases = {
            { { "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6" }, "e1g1", g1, h1, f1, PieceColors::Light },
            { { "d2d4", "d7d5", "b1c3", "b8c6", "c1f4", "c8f5", "d1d2", "d8d7" }, "e1c1", c1, a1, d1, PieceColors::Light },
            { { "e2e4", "e7e5", "g1f3", "g8f6", "f1c4", "f8c5", "b1c3" }, "e8g8", g8, h8, f8, PieceColors::Dark },
            { { "d2d4", "d7d5", "c2c4", "b8c6", "b1c3", "c8f5", "g1f3", "d8d7", "e2e3" }, "e8c8", c8, a8, d8, PieceColors::Dark },
        };
        for (const auto& c : cases) {
            const auto before = Position(c.moves);
            const auto after = Position(Plus(c.moves, c.castling));
            const Piece king{ KING, c.color }, rook{ ROOK, c.color };
            const auto kingFrom = c.color == PieceColors::Light ? e1 : e8;

            auto diff = BoardDiff(before, after);
            Assert::AreEqual(size_t(2), diff.Size());
            Assert::IsTrue(HasTravel(diff, kingFrom, c.kingTo, king, king));
            Assert::IsTrue(HasTravel(diff, c.rookFrom, c.rookTo, rook, rook));

            diff = BoardDiff(after, before);
            Assert::AreEqual(size_t(2), diff.Size());
            Assert::IsTrue(HasTravel(diff, c.kingTo, kingFrom, king, king));
            Assert::IsTrue(HasTravel(diff, c.rookTo, c.rookFrom, rook, rook));
        }
    }

    TEST_METHOD(EnPassantAndItsTakeback) {
        const std::vector<std::string> moves = { "e2e4", "a7a6", "e4e5", "d7d5" };
        const auto before = Position(moves);
        const auto after = Position(Plus(moves, "e5d6"));

        auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, e5, d6, WhitePawn, WhitePawn));
        Assert::IsTrue(Has(diff.Removed(), d5, BlackPawn));

        diff = BoardDiff(after, before);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, d6, e5, WhitePawn, WhitePawn));
        Assert::IsTrue(Has(diff.Added(), d5, BlackPawn));
    }

    TEST_METHOD(PromotionAndItsTakeback) {
        const std::vector<std::string> moves = { "h2h4", "g7g5", "h4g5", "h7h6", "g5h6", "g8f6", "h6h7", "h8g8" };
        const auto before = Position(moves);
        const auto after = Position(Plus(moves, "h7h8q"));
        const Piece queen{ QUEEN, PieceColors::Light };

        auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, h7, h8, WhitePawn, queen));

        diff = BoardDiff(after, before);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, h8, h7, queen, WhitePawn));
    }

    TEST_METHOD(PromotionWithCaptureAndItsTakeback) {
        const std::vector<std::string> moves = { "g2g4", "h7h5", "a2a3", "h5g4", "a3a4", "g4g3", "a4a5", "g3h2", "a5a6" };
        const auto before = Position(moves);
        const auto after = Position(Plus(moves, "h2g1q"));
        const Piece queen{ QUEEN, PieceColors::Dark }, knight{ KNIGHT, PieceColors::Light };

        auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, h2, g1, BlackPawn, queen));
        Assert::IsTrue(Has(diff.Removed(), g1, knight));

        diff = BoardDiff(after, before);
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, g1, h2, queen, BlackPawn));
        Assert::IsTrue(Has(diff.Added(), g1, knight));
    }

    // The player's promotion: the pawn stands on the last rank until the piece is chosen.
    TEST_METHOD(PromotionChoiceInPlace) {
        Board before;
        before.Initialize();
        Board after = before;
        before.Place(h8, WhitePawn);
        after.Place(h8, { ROOK, PieceColors::Light });

        const auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, h8, h8, WhitePawn, { ROOK, PieceColors::Light }));
    }

    // Lichess can send several moves at once.
    TEST_METHOD(TwoMovesAtOnce) {
        const auto diff = BoardDiff(Position({}), Position({ "e2e4", "e7e5" }));
        Assert::AreEqual(size_t(2), diff.Size());
        Assert::IsTrue(HasTravel(diff, e2, e4, WhitePawn, WhitePawn));
        Assert::IsTrue(HasTravel(diff, e7, e5, BlackPawn, BlackPawn));
    }

    // Of two equal pieces that could have gone, the nearer one did.
    TEST_METHOD(NearestEqualPiece) {
        Board before;
        before.Initialize();
        for (int i = 0; i < BpMax; ++i)
            before.Place(BoardPosition(i), {});
        const Piece rook{ ROOK, PieceColors::Light };
        before.Place(a1, rook);
        before.Place(h1, rook);
        Board after = before;
        after.Place(h1, {});
        after.Place(h5, rook);

        const auto diff = BoardDiff(before, after);
        Assert::AreEqual(size_t(1), diff.Size());
        Assert::IsTrue(HasTravel(diff, h1, h5, rook, rook));
    }

    // Back to the start from a middlegame: many changes, which the caller shows without slides.
    TEST_METHOD(NewGameIsABigChange) {
        const auto middlegame = Position({ "e2e4", "d7d5", "e4d5", "d8d5", "b1c3", "d5a5", "d2d4", "g8f6",
                                           "g1f3", "c8f5", "f1c4", "e7e6", "c1d2", "c7c6" });
        Assert::IsTrue(BoardDiff(middlegame, Position({})).Size() > 4);
    }
};
}
