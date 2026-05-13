#include "stdafx.h"
#include <CppUnitTest.h>
#include <CppUnitTestAssert.h>
#include "Move.h"
#include "Game.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;
using namespace std;

namespace ChessTests
{
	TEST_CLASS(FenGenTests)
	{
	public:

		TEST_METHOD(GameFenCapture_Test)
		{
			// 1.e4 d5 2.exd5 — capture resets halfmove clock to 0
			string fen = "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2";
			auto game = std::make_unique<Game>();

			game->DoMove({ e2, e4 });
			game->DoMove({ d7, d5 });
			game->DoMove({ e4, d5 }); // capture

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenEnPassantTarget_Test)
		{
			// 1.d4 e5 2.d5 c5 — black's double push gives white en passant on c6
			string fen = "rnbqkbnr/pp1p1ppp/8/2pPp3/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ d2, d4 });
			game->DoMove({ e7, e5 });
			game->DoMove({ d4, d5 });
			game->DoMove({ c7, c5 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenHalfMoveClock_Test)
		{
			// 1.Nf3 Nf6 2.Ng1 Ng8 — halfmove clock is 4 even though the board looks like the start
			string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 4 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ g1, f3 });
			game->DoMove({ g8, f6 });
			game->DoMove({ f3, g1 });
			game->DoMove({ f6, g8 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenCastlingLost_Test)
		{
			// 1.a4 a5 2.Ra3 — white a1 rook moves; only queenside right lost, kingside survives
			string fen = "rnbqkbnr/1ppppppp/8/p7/P7/R7/1PPPPPPP/1NBQKBNR b Kkq - 1 2";
			auto game = std::make_unique<Game>();

			game->DoMove({ a2, a4 });
			game->DoMove({ a7, a5 });
			game->DoMove({ a1, a3 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenRuyLopez_Test)
		{
			// 1.e4 e5 2.Nf3 Nc6 3.Bb5 — Ruy Lopez; halfmove=3, fullmove=3
			string fen = "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ e2, e4 });
			game->DoMove({ e7, e5 });
			game->DoMove({ g1, f3 });
			game->DoMove({ b8, c6 });
			game->DoMove({ f1, b5 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenPawnPromotion_Test)
		{
			// White b-pawn captures its way to the 7th rank, then promotes to queen on b8.
			// 1.b4 c5 2.bxc5 b6 3.cxb6 a5 4.b7 Na6 5.b8=Q
			string fen = "rQbqkbnr/3ppppp/n7/p7/8/8/P1PPPPPP/RNBQKBNR b KQkq - 0 5";
			auto game = std::make_unique<Game>();

			game->DoMove({ b2, b4 });        // 1. b4
			game->DoMove({ c7, c5 });        // 1... c5
			game->DoMove({ b4, c5 });        // 2. bxc5
			game->DoMove({ b7, b6 });        // 2... b6
			game->DoMove({ c5, b6 });        // 3. cxb6
			game->DoMove({ a7, a5 });        // 3... a5
			game->DoMove({ b6, b7 });        // 4. b7
			game->DoMove({ b8, a6 });        // 4... Na6 (clears b8)
			game->DoMove(Move::Parse("b7b8q")); // 5. b8=Q

			Assert::AreEqual(fen, game->MakeFen());
		}

		// ── Long-game FEN tests (25 – 65 half-moves) ──────────────────────

		TEST_METHOD(GameFenKasparovTopalov1999_25_Test)
		{
			// Kasparov vs Topalov, Wijk aan Zee 1999 — after White's 13th move (Nc1)
			// White castled queenside (king left e1) → all white rights gone; Black untouched → kq
			string fen = "r3k2r/1b1nqp1p/p1pp1npQ/1p2p3/3PP3/P1N2P2/1PP3PP/1KNR1B1R b kq - 1 13";
			auto game = std::make_unique<Game>();

			for (const auto& uci : {
				"e2e4","d7d6","d2d4","g8f6","b1c3","g7g6","c1e3","f8g7",
				"d1d2","c7c6","f2f3","b7b5","g1e2","b8d7","e3h6","g7h6",
				"d2h6","c8b7","a2a3","e7e5","e1c1","d8e7","c1b1","a7a6","e2c1"
				})
				game->DoMove(Move::Parse(std::string(uci)));

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenFischerSpassky1972G6_35_Test)
		{
			// Fischer vs Spassky, World Championship 1972 Game 6 — after White's 18th move (Nd4)
			// Rook left a1 (hm 21) + king castled (hm 31) → no white rights; black castled (hm 10) → neither
			string fen = "2r3k1/r2nqpp1/p3b2p/2pp4/3N4/Q3P3/PP2BPPP/2R2RK1 b - - 5 18";
			auto game = std::make_unique<Game>();

			for (const auto& uci : {
				"c2c4","e7e6","g1f3","d7d5","d2d4","g8f6","b1c3","f8e7",
				"c1g5","e8g8","e2e3","h7h6","g5h4","b7b6","c4d5","f6d5",
				"h4e7","d8e7","c3d5","e6d5","a1c1","c8e6","d1a4","c7c5",
				"a4a3","f8c8","f1b5","a7a6","d4c5","b6c5","e1g1","a8a7",
				"b5e2","b8d7","f3d4"
				})
				game->DoMove(Move::Parse(std::string(uci)));

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenDeepBlueKasparov1997G2_57_Test)
		{
			// Deep Blue vs Kasparov, World Championship 1997 Game 2 — after White's 29th move (Qf2)
			// Both sides castled early → neither can castle → "-"
			string fen = "r1rqnbk1/3b1pp1/p6p/1p1Pp3/PPp1P3/R1P1B1NP/2B2QP1/R5K1 b - - 3 29";
			auto game = std::make_unique<Game>();

			for (const auto& uci : {
				"e2e4","e7e5","g1f3","b8c6","f1b5","a7a6","b5a4","g8f6",
				"e1g1","f8e7","f1e1","b7b5","a4b3","d7d6","c2c3","e8g8",
				"h2h3","h7h6","d2d4","f8e8","b1d2","e7f8","d2f1","c8d7",
				"f1g3","c6a5","b3c2","c7c5","b2b3","a5c6","d4d5","c6e7",
				"c1e3","e7g6","d1d2","f6h7","a2a4","g6h4","f3h4","d8h4",
				"d2e2","h4d8","b3b4","d8c7","e1c1","c5c4","a1a3","e8c8",
				"c1a1","c7d8","f2f4","h7f6","f4e5","d6e5","e2f1","f6e8","f1f2"
				})
				game->DoMove(Move::Parse(std::string(uci)));

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenByrne_FischerGameOfCentury1956_61_Test)
		{
			// Byrne vs Fischer, New York 1956 "Game of the Century" — after White's 31st move (Nf3)
			// White rook left a1 (hm 17), king left e1 (hm 33); Black castled (hm 8) → neither can castle
			string fen = "3Q1bk1/1p3p1p/2p3p1/3b4/8/5N1P/r4nPK/8 b - - 2 31";
			auto game = std::make_unique<Game>();

			for (const auto& uci : {
				"g1f3","g8f6","c2c4","g7g6","b1c3","f8g7","d2d4","e8g8",
				"c1f4","d7d5","d1b3","d5c4","b3c4","c7c6","e2e4","b8d7",
				"a1d1","d7b6","c4c5","c8g4","f4g5","b6a4","c5a3","a4c3",
				"b2c3","f6e4","g5e7","d8b6","f1c4","e4c3","e7c5","f8e8",
				"e1f1","g4e6","c5b6","e6c4","f1g1","c3e2","g1f1","e2d4",
				"f1g1","d4e2","g1f1","e2c3","f1g1","a7b6","a3b4","a8a4",
				"b4b6","c3d1","h2h3","a4a2","g1h2","d1f2","h1e1","e8e1",
				"b6d8","g7f8","f3e1","c4d5","e1f3"
				})
				game->DoMove(Move::Parse(std::string(uci)));

			Assert::AreEqual(fen, game->MakeFen());
		}

		TEST_METHOD(GameFenDeepBlueKasparov1997G2_65_Test)
		{
			// Deep Blue vs Kasparov, World Championship 1997 Game 2 — after White's 33rd move (Nf5)
			// Both sides castled early → neither can castle → "-"
			string fen = "r1r1qbk1/3b1pp1/p2n3p/1pBPpN2/PPp1P3/2P4P/R1B2QP1/R5K1 b - - 11 33";
			auto game = std::make_unique<Game>();

			for (const auto& uci : {
				"e2e4","e7e5","g1f3","b8c6","f1b5","a7a6","b5a4","g8f6",
				"e1g1","f8e7","f1e1","b7b5","a4b3","d7d6","c2c3","e8g8",
				"h2h3","h7h6","d2d4","f8e8","b1d2","e7f8","d2f1","c8d7",
				"f1g3","c6a5","b3c2","c7c5","b2b3","a5c6","d4d5","c6e7",
				"c1e3","e7g6","d1d2","f6h7","a2a4","g6h4","f3h4","d8h4",
				"d2e2","h4d8","b3b4","d8c7","e1c1","c5c4","a1a3","e8c8",
				"c1a1","c7d8","f2f4","h7f6","f4e5","d6e5","e2f1","f6e8",
				"f1f2","e8d6","e3b6","d8e8","a3a2","f8e7","b6c5","e7f8","g3f5"
				})
				game->DoMove(Move::Parse(std::string(uci)));

			Assert::AreEqual(fen, game->MakeFen());
		}
		// ── Bug documentation — these tests assert CORRECT standard FEN behaviour ──
		// ── and are EXPECTED TO FAIL until the two castling bugs are fixed.      ──
		//
		// Bug 1: IsCastlingPossible returns a single bool per side, so the castling
		//        field is always "KQ"or"-" for white concatenated with "kq"or"-" for
		//        black.  This produces "KQ-", "-kq", and "--" which are invalid FEN;
		//        Stockfish would reject them.  The correct values are "KQ", "kq", "-".
		//
		// Bug 2: IsCastlingPossible checks whether ANY piece moved from {a1,e1,h1}
		//        (or {a8,e8,h8}), so moving only one rook strips the castling right
		//        for BOTH sides of that colour.  Standard FEN keeps the other side's
		//        right alive (e.g. "K" when only the a1 rook has moved).

		// Bug 1 — neither side can castle: "--" produced, "-" required
		TEST_METHOD(GameFenCastling_NeitherSide_SingleDash_Test)
		{
			// 1.e4 e5 2.Ke2 Ke7 — both kings leave their home squares
			// current output: "w --"   correct standard FEN: "w -"
			string fen = "rnbq1bnr/ppppkppp/8/4p3/4P3/8/PPPPKPPP/RNBQ1BNR w - - 2 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ e2, e4 });
			game->DoMove({ e7, e5 });
			game->DoMove({ e1, e2 });
			game->DoMove({ e8, e7 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		// Bug 1 — only white can castle: "KQ-" produced, "KQ" required
		TEST_METHOD(GameFenCastling_OnlyWhite_NoTrailingDash_Test)
		{
			// 1.e4 e5 2.Nf3 Ke7 — black king moves, white king and rooks untouched
			// current output: "w KQ-"   correct standard FEN: "w KQ"
			string fen = "rnbq1bnr/ppppkppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQ - 2 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ e2, e4 });
			game->DoMove({ e7, e5 });
			game->DoMove({ g1, f3 });
			game->DoMove({ e8, e7 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		// Bug 1 — only black can castle: "-kq" produced, "kq" required
		TEST_METHOD(GameFenCastling_OnlyBlack_NoLeadingDash_Test)
		{
			// 1.e4 e5 2.Ke2 Nf6 — white king moves, black king and rooks untouched
			// current output: "w -kq"   correct standard FEN: "w kq"
			string fen = "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/8/PPPPKPPP/RNBQ1BNR w kq - 2 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ e2, e4 });
			game->DoMove({ e7, e5 });
			game->DoMove({ e1, e2 });
			game->DoMove({ g8, f6 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		// Bug 2 — only queenside rook (a1) moved: kingside right "K" must survive
		TEST_METHOD(GameFenCastling_QueensideRookMoved_KingsideRightPreserved_Test)
		{
			// 1.a4 a5 2.Ra3 Nf6 — white a1 rook moves; king and h1 rook untouched
			// current output: "w -kq"   correct standard FEN: "w Kkq"
			string fen = "rnbqkb1r/1ppppppp/5n2/p7/P7/R7/1PPPPPPP/1NBQKBNR w Kkq - 2 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ a2, a4 });
			game->DoMove({ a7, a5 });
			game->DoMove({ a1, a3 });
			game->DoMove({ g8, f6 });

			Assert::AreEqual(fen, game->MakeFen());
		}

		// Bug 2 — only kingside rook (h1) moved: queenside right "Q" must survive
		TEST_METHOD(GameFenCastling_KingsideRookMoved_QueensideRightPreserved_Test)
		{
			// 1.h4 h5 2.Rh3 Nf6 — white h1 rook moves; king and a1 rook untouched
			// current output: "w -kq"   correct standard FEN: "w Qkq"
			string fen = "rnbqkb1r/ppppppp1/5n2/7p/7P/7R/PPPPPPP1/RNBQKBN1 w Qkq - 2 3";
			auto game = std::make_unique<Game>();

			game->DoMove({ h2, h4 });
			game->DoMove({ h7, h5 });
			game->DoMove({ h1, h3 });
			game->DoMove({ g8, f6 });

			Assert::AreEqual(fen, game->MakeFen());
		}
	};
}
