#include "stdafx.h"
#include <CppUnitTest.h>
#include "../ChessCore/Board.h"
#include "../ChessCore/Move.h"
#include "../ChessCore/San.h"

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

} // namespace ChessTests
