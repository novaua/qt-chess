#include "stdafx.h"
#include <CppUnitTest.h>
#include "../ChessCore/Board.h"
#include "../ChessCore/Move.h"
#include "../ChessCore/San.h"
#include "../ChessCore/Game.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

namespace ChessTests {

static void ClearBoard(Board& board) {
    for (int i = 0; i < BpMax; ++i)
        board.Place(BoardPosition(i), { EMPTY, PieceColors::Empty });
}

TEST_CLASS(SanTests) {
public:

    // -------------------------------------------------------------------------
    // Pawn moves
    // -------------------------------------------------------------------------

    TEST_METHOD(PawnAdvance_e4) {
        Board board; board.Initialize();
        auto hm = board.DoMove({ e2, e4, false });
        Assert::AreEqual(std::string("e4"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PawnAdvance_d4) {
        Board board; board.Initialize();
        auto hm = board.DoMove({ d2, d4, false });
        Assert::AreEqual(std::string("d4"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PawnCapture_exd5) {
        Board board; ClearBoard(board);
        board.Place(e4, { PAWN, PieceColors::Light });
        board.Place(d5, { PAWN, PieceColors::Dark });
        auto hm = board.DoMove({ e4, d5, true });
        Assert::AreEqual(std::string("exd5"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(EnPassant_exd6) {
        Board board; ClearBoard(board);
        board.Place(e5, { PAWN, PieceColors::Light });
        board.Place(d5, { PAWN, PieceColors::Dark  });
        auto hm = board.DoMove({ e5, d6, true });
        Assert::AreEqual(std::string("exd6"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PawnPromotion_e8Q) {
        Board board; ClearBoard(board);
        board.Place(e7, { PAWN, PieceColors::Light });
        auto hm = board.DoMove({ e7, e8, false, { QUEEN, PieceColors::Light } });
        Assert::AreEqual(std::string("e8=Q"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PawnPromotionWithCheck_e8Qplus) {
        Board board; ClearBoard(board);
        board.Place(e7, { PAWN, PieceColors::Light });
        board.Place(e1, { KING, PieceColors::Dark  });
        auto hm = board.DoMove({ e7, e8, false, { QUEEN, PieceColors::Light } });
        Assert::AreEqual(std::string("e8=Q+"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PawnPromotion_captureAndPromote) {
        Board board; ClearBoard(board);
        board.Place(d7, { PAWN, PieceColors::Light });
        board.Place(e8, { ROOK, PieceColors::Dark  });
        auto hm = board.DoMove({ d7, e8, true, { QUEEN, PieceColors::Light } });
        Assert::AreEqual(std::string("dxe8=Q"), FormatMoveSan(hm, board, false));
    }

    // -------------------------------------------------------------------------
    // Piece moves
    // -------------------------------------------------------------------------

    TEST_METHOD(KnightMove_Nf3) {
        Board board; board.Initialize();
        auto hm = board.DoMove({ g1, f3, false });
        Assert::AreEqual(std::string("Nf3"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(BishopMove_Bc4) {
        Board board; board.Initialize();
        board.DoMove({ e2, e4, false });
        board.DoMove({ e7, e5, false });
        board.DoMove({ g1, f3, false });
        auto hm = board.DoMove({ f1, c4, false });
        Assert::AreEqual(std::string("Bc4"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(PieceCapture_Nxe5) {
        Board board; ClearBoard(board);
        board.Place(f3, { KNIGHT, PieceColors::Light });
        board.Place(e5, { PAWN,   PieceColors::Dark  });
        auto hm = board.DoMove({ f3, e5, true });
        Assert::AreEqual(std::string("Nxe5"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(RookCapture_Rxd5) {
        Board board; ClearBoard(board);
        board.Place(d1, { ROOK, PieceColors::Light });
        board.Place(d5, { PAWN, PieceColors::Dark  });
        auto hm = board.DoMove({ d1, d5, true });
        Assert::AreEqual(std::string("Rxd5"), FormatMoveSan(hm, board, false));
    }

    // -------------------------------------------------------------------------
    // Castling
    // -------------------------------------------------------------------------

    TEST_METHOD(CastlingKingside_OO) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING, PieceColors::Light });
        board.Place(h1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, g1, false });
        Assert::AreEqual(std::string("O-O"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(CastlingQueenside_OOO) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING, PieceColors::Light });
        board.Place(a1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, c1, false });
        Assert::AreEqual(std::string("O-O-O"), FormatMoveSan(hm, board, false));
    }

    // -------------------------------------------------------------------------
    // Disambiguation
    // -------------------------------------------------------------------------

    TEST_METHOD(DisambigByFile_Rae1) {
        Board board; ClearBoard(board);
        board.Place(a1, { ROOK, PieceColors::Light });
        board.Place(d1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ a1, e1, false });
        Assert::AreEqual(std::string("Rae1"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(DisambigByFile_Rde1) {
        // a1 rook is blocked by d1; d1 moves to e1 unambiguously → Re1
        Board board; ClearBoard(board);
        board.Place(a1, { ROOK, PieceColors::Light });
        board.Place(d1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ d1, e1, false });
        Assert::AreEqual(std::string("Re1"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(DisambigByRank_R1e4) {
        Board board; ClearBoard(board);
        board.Place(e1, { ROOK, PieceColors::Light });
        board.Place(e2, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, e4, false });
        Assert::AreEqual(std::string("R1e4"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(DisambigBoth_Nef6) {
        // Two knights on e4 and g4 both reach f6
        Board board; ClearBoard(board);
        board.Place(e4, { KNIGHT, PieceColors::Light });
        board.Place(g4, { KNIGHT, PieceColors::Light });
        auto hm = board.DoMove({ e4, f6, false });
        Assert::AreEqual(std::string("Nef6"), FormatMoveSan(hm, board, false));
    }

    // -------------------------------------------------------------------------
    // Check and checkmate suffixes
    // -------------------------------------------------------------------------

    TEST_METHOD(CheckSuffix) {
        Board board; ClearBoard(board);
        board.Place(h1, { QUEEN, PieceColors::Light });
        board.Place(h8, { KING,  PieceColors::Dark  });
        auto hm = board.DoMove({ h1, h7, false });
        Assert::AreEqual(std::string("Qh7+"), FormatMoveSan(hm, board, false));
    }

    TEST_METHOD(CheckmateSuffix) {
        Board board; ClearBoard(board);
        board.Place(h1, { QUEEN, PieceColors::Light });
        board.Place(h8, { KING,  PieceColors::Dark  });
        auto hm = board.DoMove({ h1, h7, false });
        Assert::AreEqual(std::string("Qh7#"), FormatMoveSan(hm, board, true));
    }

    // -------------------------------------------------------------------------
    // IsCastling / IsEnPassant predicates
    // -------------------------------------------------------------------------

    TEST_METHOD(IsCastling_True_Kingside) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING, PieceColors::Light });
        board.Place(h1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, g1, false });
        Assert::IsTrue(hm.IsCastlingMove());
    }

    TEST_METHOD(IsCastling_True_Queenside) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING, PieceColors::Light });
        board.Place(a1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, c1, false });
        Assert::IsTrue(hm.IsCastlingMove());
    }

    TEST_METHOD(IsCastling_False_KingStep) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING, PieceColors::Light });
        auto hm = board.DoMove({ e1, d1, false });
        Assert::IsFalse(hm.IsCastlingMove());
    }

    TEST_METHOD(IsCastling_False_RegularMove) {
        Board board; ClearBoard(board);
        board.Place(e1, { ROOK, PieceColors::Light });
        auto hm = board.DoMove({ e1, e4, false });
        Assert::IsFalse(hm.IsCastlingMove());
    }

    TEST_METHOD(IsEnPassant_True) {
        Board board; ClearBoard(board);
        board.Place(e5, { PAWN, PieceColors::Light });
        board.Place(d5, { PAWN, PieceColors::Dark  });
        auto hm = board.DoMove({ e5, d6, true }); // d6 is empty
        Assert::IsTrue(hm.IsEnPassantMove());
    }

    TEST_METHOD(IsEnPassant_False_NormalCapture) {
        Board board; ClearBoard(board);
        board.Place(e4, { PAWN, PieceColors::Light });
        board.Place(d5, { PAWN, PieceColors::Dark  });
        auto hm = board.DoMove({ e4, d5, true });
        Assert::IsFalse(hm.IsEnPassantMove());
    }

    TEST_METHOD(IsEnPassant_False_PawnAdvance) {
        Board board; ClearBoard(board);
        board.Place(e2, { PAWN, PieceColors::Light });
        auto hm = board.DoMove({ e2, e4, false });
        Assert::IsFalse(hm.IsEnPassantMove());
    }
};

static MovesHistory PlayMoves(const std::vector<std::pair<BoardPosition, BoardPosition>>& moves) {
    Game game;
    game.Restart();
    for (const auto& [from, to] : moves)
        game.DoMove(from, to);
    return game.GetGameRecord();
}

TEST_CLASS(HistorySanTests) {
public:

    TEST_METHOD(FoolsMate_LastMoveIsMate) {
        auto sans = Game::FormatHistorySan(PlayMoves({ { f2, f3 }, { e7, e5 }, { g2, g4 }, { d8, h4 } }));
        Assert::AreEqual<size_t>(4u, sans.size());
        Assert::AreEqual(std::string("f3"),   sans[0]);
        Assert::AreEqual(std::string("e5"),   sans[1]);
        Assert::AreEqual(std::string("g4"),   sans[2]);
        Assert::AreEqual(std::string("Qh4#"), sans[3]);
    }

    TEST_METHOD(CheckIsNotMate) {
        auto sans = Game::FormatHistorySan(PlayMoves({ { e2, e4 }, { f7, f6 }, { d1, h5 } }));
        Assert::AreEqual(std::string("Qh5+"), sans[2]);
    }

    TEST_METHOD(CaptureAndCastling) {
        auto sans = Game::FormatHistorySan(PlayMoves({
            { e2, e4 }, { d7, d5 }, { e4, d5 }, { g8, f6 },
            { g1, f3 }, { f6, d5 }, { f1, c4 }, { c8, g4 }, { e1, g1 } }));
        Assert::AreEqual(std::string("exd5"), sans[2]);
        Assert::AreEqual(std::string("Nxd5"), sans[5]);
        Assert::AreEqual(std::string("O-O"),  sans[8]);
    }

    TEST_METHOD(EmptyHistory) {
        Assert::AreEqual<size_t>(0u, Game::FormatHistorySan({}).size());
    }
};

TEST_CLASS(PgnTests) {
public:

    TEST_METHOD(TagsMovesAndResult) {
        auto pgn = FormatPgn({ { "White", "Alice" }, { "Black", "Bob" }, { "Result", "0-1" } },
            { "f3", "e5", "g4", "Qh4#" }, "0-1");
        Assert::AreEqual(std::string(
            "[White \"Alice\"]\n"
            "[Black \"Bob\"]\n"
            "[Result \"0-1\"]\n"
            "\n"
            "1. f3 e5 2. g4 Qh4# 0-1\n"), pgn);
    }

    TEST_METHOD(OddMoveCountAndUnfinished) {
        auto pgn = FormatPgn({}, { "e4", "e5", "Nf3" }, "");
        Assert::AreEqual(std::string("\n1. e4 e5 2. Nf3 *\n"), pgn);
    }

    TEST_METHOD(TagValuesAreEscaped) {
        auto pgn = FormatPgn({ { "White", "Al \"the\" \\ king" } }, {}, "*");
        Assert::AreEqual(std::string("[White \"Al \\\"the\\\" \\\\ king\"]\n\n*\n"), pgn);
    }

    TEST_METHOD(LongGameWrapsAt80Columns) {
        std::vector<std::string> moves;
        for (int i = 0; i < 120; ++i)
            moves.push_back(i % 2 == 0 ? "Nf3" : "Ng8");
        auto pgn = FormatPgn({}, moves, "1/2-1/2");

        std::string joined;
        size_t start = 1; // after the blank line that ends the (empty) tag section
        while (start < pgn.size()) {
            auto end = pgn.find('\n', start);
            auto line = pgn.substr(start, end - start);
            Assert::IsTrue(line.size() <= 80, L"line longer than 80 characters");
            joined += (joined.empty() ? "" : " ") + line;
            start = end + 1;
        }
        Assert::IsTrue(joined.rfind("60. Nf3 Ng8 1/2-1/2") == joined.size() - 19);
        Assert::IsTrue(joined.find("1. Nf3 Ng8 2. Nf3") == 0);
    }
};

TEST_CLASS(MaterialTests) {
public:

    TEST_METHOD(StartPositionIsEven) {
        Board board;
        Assert::AreEqual(0, MaterialBalance(board));
    }

    TEST_METHOD(CapturedPawnCounts) {
        Game game;
        game.Restart();
        game.DoMove(e2, e4);
        game.DoMove(d7, d5);
        game.DoMove(e4, d5);
        Assert::AreEqual(1, MaterialBalance(game.GetCurrentBoard()));
    }

    TEST_METHOD(BlackAheadIsNegative) {
        Board board; ClearBoard(board);
        board.Place(e1, { KING,   PieceColors::Light });
        board.Place(a1, { KNIGHT, PieceColors::Light });
        board.Place(e8, { KING,   PieceColors::Dark  });
        board.Place(a8, { ROOK,   PieceColors::Dark  });
        Assert::AreEqual(-2, MaterialBalance(board));
    }

    TEST_METHOD(PromotedPieceCountsAtNewValue) {
        Board board; ClearBoard(board);
        board.Place(e7, { PAWN, PieceColors::Light });
        board.Place(a8, { ROOK, PieceColors::Dark  });
        Assert::AreEqual(-4, MaterialBalance(board));
        board.DoMove({ e7, e8, false, { QUEEN, PieceColors::Light } });
        Assert::AreEqual(4, MaterialBalance(board));
    }

    TEST_METHOD(PieceValues) {
        Assert::AreEqual(1, PieceValue(PAWN));
        Assert::AreEqual(3, PieceValue(KNIGHT));
        Assert::AreEqual(3, PieceValue(BISHOP));
        Assert::AreEqual(5, PieceValue(ROOK));
        Assert::AreEqual(9, PieceValue(QUEEN));
        Assert::AreEqual(0, PieceValue(KING));
    }
};

} // namespace ChessTests
