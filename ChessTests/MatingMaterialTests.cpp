#include "stdafx.h"
#include <CppUnitTest.h>
#include "../ChessCore/Board.h"
#include "../ChessCore/Game.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

namespace ChessTests {

// Board::CanMate (LI-02): whether a flag loses (the other side can mate) or draws.
TEST_CLASS(MatingMaterialTests)
{
    static constexpr PieceColors White = PieceColors::Light;
    static constexpr PieceColors Black = PieceColors::Dark;

    // A board with only these pieces, e.g. { {e1, KING, White}, {e8, KING, Black} }.
    struct Placed { BoardPosition position; PieceTypes type; PieceColors color; };
    static Board With(std::initializer_list<Placed> pieces)
    {
        Board board;
        for (int i = 0; i < BpMax; ++i)
            board.Place(BoardPosition(i), { EMPTY, PieceColors::Empty });
        for (const auto& p : pieces)
            board.Place(p.position, { p.type, p.color });
        return board;
    }

public:
    // The shortest known stalemate (Sam Loyd). A mate isn't one.
    TEST_METHOD(Game_IsStalemate)
    {
        Game game;
        game.Restart();
        for (const char* move : { "e2e3", "a7a5", "d1h5", "a8a6", "h5a5", "h7h5", "h2h4", "a6h6", "a5c7", "f7f6",
                                  "c7d7", "e8f7", "d7b7", "d8d3", "b7b8", "d3h7", "b8c8", "f7g6" })
            game.DoMove(Move::Parse(move));
        Assert::IsFalse(game.IsStalemate());
        game.DoMove(Move::Parse("c8e6"));
        Assert::IsTrue(game.IsStalemate());

        Game mated;
        mated.Restart();
        for (const char* move : { "f2f3", "e7e5", "g2g4", "d8h4" })
            mated.DoMove(Move::Parse(move));
        Assert::IsFalse(mated.IsStalemate());
    }

    TEST_METHOD(StartPosition_BothCanMate)
    {
        Board board;
        Assert::IsTrue(board.CanMate(White));
        Assert::IsTrue(board.CanMate(Black));
    }

    TEST_METHOD(LoneKing_CantMate)
    {
        const auto board = With({ { e1, KING, White }, { e8, KING, Black }, { d8, QUEEN, Black } });
        Assert::IsFalse(board.CanMate(White));
        Assert::IsTrue(board.CanMate(Black));
    }

    TEST_METHOD(PawnRookOrQueen_CanMate)
    {
        for (auto type : { PAWN, ROOK, QUEEN }) {
            const auto board = With({ { e1, KING, White }, { e8, KING, Black }, { a2, type, White } });
            Assert::IsTrue(board.CanMate(White));
        }
    }

    TEST_METHOD(KingAndKnight_MatesOnlyWithTheOtherSidesHelp)
    {
        Assert::IsFalse(With({ { e1, KING, White }, { e8, KING, Black }, { b1, KNIGHT, White } }).CanMate(White));
        // A black pawn can block its own king's flight square.
        Assert::IsTrue(With({ { e1, KING, White }, { h8, KING, Black }, { b1, KNIGHT, White }, { h7, PAWN, Black } })
            .CanMate(White));
        Assert::IsTrue(With({ { e1, KING, White }, { h8, KING, Black }, { b1, KNIGHT, White }, { g8, ROOK, Black } })
            .CanMate(White));
    }

    // Lichess's rule: a queen doesn't help, it takes the knight.
    TEST_METHOD(KingAndKnight_CantMateAgainstQueensOnly)
    {
        Assert::IsFalse(With({ { e1, KING, White }, { e8, KING, Black }, { b1, KNIGHT, White }, { d8, QUEEN, Black } })
            .CanMate(White));
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { b1, KNIGHT, White }, { d8, QUEEN, Black },
            { a8, ROOK, Black } }).CanMate(White));
    }

    TEST_METHOD(TwoKnightsOrKnightAndBishop_CanMate)
    {
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { b1, KNIGHT, White }, { g1, KNIGHT, White } })
            .CanMate(White));
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { b1, KNIGHT, White }, { c1, BISHOP, White } })
            .CanMate(White));
    }

    TEST_METHOD(BishopsOnOneSquareColour_CantMateALoneKing)
    {
        // c1 and e3 are both dark squares.
        Assert::IsFalse(With({ { e1, KING, White }, { e8, KING, Black }, { c1, BISHOP, White }, { e3, BISHOP, White } })
            .CanMate(White));
        // f1 is light: a pair on both colours mates.
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { c1, BISHOP, White }, { f1, BISHOP, White } })
            .CanMate(White));
    }

    TEST_METHOD(Bishop_AgainstBishops_DependsOnTheirSquareColour)
    {
        // c1 (dark) against c8 (light): opposite colours, a mate is possible.
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { c1, BISHOP, White }, { c8, BISHOP, Black } })
            .CanMate(White));
        // c1 against f8 (dark): the same colour, never.
        Assert::IsFalse(With({ { e1, KING, White }, { e8, KING, Black }, { c1, BISHOP, White }, { f8, BISHOP, Black } })
            .CanMate(White));
        // A black knight or pawn can block a square.
        Assert::IsTrue(With({ { e1, KING, White }, { e8, KING, Black }, { c1, BISHOP, White }, { g8, KNIGHT, Black } })
            .CanMate(White));
    }
};

}
