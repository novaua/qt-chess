#include "stdafx.h"
#include <CppUnitTest.h>
#include <set>
#include <sstream>
#include "../ChessCore/Game.h"
#include "../ChessCore/BoardDiff.h"
#include "../ChessCore/ChessException.h"
#include "Chess960Reference.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Chess;

// LI-01A Chess960, step 1: the start positions and castling from any set-up
// (docs/LI-01A-Chess960Plan.md §3.1–3.5).
namespace ChessTests {

static std::vector<std::string> Words(const std::string& text)
{
    std::vector<std::string> words;
    std::istringstream in(text);
    for (std::string w; in >> w;)
        words.push_back(w);
    return words;
}

static std::string RankText(const StartPosition& start)
{
    static const char letters[] = " NBRQKP";
    std::string text;
    for (auto type : start.BackRank())
        text += letters[type];
    return text;
}

// The legal moves of the side to move, as distinct from-to pairs (a promotion is one move here).
static size_t LegalCount(Game& game)
{
    std::set<std::pair<int, int>> moves;
    for (int square = 0; square < BpMax; ++square)
        for (const auto& m : game.GetAllowedMoves(square))
            moves.insert({ m.From, m.To });
    return moves.size();
}

static std::shared_ptr<Game> Play960(int number, const std::vector<std::string>& moves)
{
    auto game = std::make_shared<Game>();
    game->Restart(true, StartPosition::Chess960(number));
    for (const auto& m : moves)
        game->DoMove(Move::Parse(m));
    return game;
}

static bool CanPlay(Game& game, BoardPosition from, BoardPosition to)
{
    const auto moves = game.GetAllowedMoves(from);
    return std::any_of(moves.begin(), moves.end(), [to](const Move& m) { return m.To == to; });
}

static const Piece WhiteKing{ KING, PieceColors::Light };
static const Piece WhiteRook{ ROOK, PieceColors::Light };
static const Piece Nothing{ EMPTY, PieceColors::Empty };

TEST_CLASS(StartPositionTests) {
public:
    TEST_METHOD(Numbering_AsLichess) {
        Assert::AreEqual(std::string("RNBQKBNR"), RankText(StartPosition::Chess960(518)));
        Assert::AreEqual(std::string("BBQNNRKR"), RankText(StartPosition::Chess960(0)));
        Assert::AreEqual(std::string("RKRNNQBB"), RankText(StartPosition::Chess960(959)));
        Assert::AreEqual(std::string("BQNNRKRB"), RankText(StartPosition::Chess960(3)));
        Assert::AreEqual(std::string("RNBQKBNR"), RankText(StartPosition::Standard()));
    }

    // Every one of the 960: bishops on squares of both colours, the king between the rooks, all
    // different, and its FEN read back to the same number.
    TEST_METHOD(AllPositions_ValidAndDifferent) {
        std::set<std::string> seen;
        for (int n = 0; n < StartPosition::Count; ++n) {
            const auto start = StartPosition::Chess960(n);
            const auto text = RankText(start);
            const auto label = std::to_wstring(n);
            Assert::AreEqual(n, start.Number(), label.c_str());
            Assert::IsTrue(start.IsChess960(), label.c_str());

            std::vector<int> bishops, rooks;
            for (int f = 0; f < 8; ++f) {
                if (start.At(f) == BISHOP) bishops.push_back(f);
                if (start.At(f) == ROOK) rooks.push_back(f);
            }
            Assert::AreEqual<size_t>(2, bishops.size(), label.c_str());
            Assert::AreNotEqual(bishops[0] % 2, bishops[1] % 2, label.c_str());
            Assert::AreEqual<size_t>(2, rooks.size(), label.c_str());
            Assert::IsTrue(rooks[0] < start.KingFile() && start.KingFile() < rooks[1], label.c_str());
            Assert::AreEqual(rooks[0], start.RookFile(false), label.c_str());
            Assert::AreEqual(rooks[1], start.RookFile(true), label.c_str());
            Assert::IsTrue(seen.insert(text).second, label.c_str());

            const auto read = StartPosition::FromFen(start.Fen());
            Assert::IsTrue(read.has_value(), label.c_str());
            Assert::AreEqual(n, read->Number(), label.c_str());
        }
    }

    TEST_METHOD(Fen_OfAStart) {
        Assert::AreEqual(std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), StartPosition::Standard().Fen());
        Assert::AreEqual(std::string("bbqnnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRKR w KQkq - 0 1"), StartPosition::Chess960(0).Fen());

        Game game;
        game.Restart(true, StartPosition::Chess960(0));
        Assert::AreEqual(StartPosition::Chess960(0).Fen(), game.MakeFen());
    }

    TEST_METHOD(FromFen_OnlyAStartPosition) {
        Assert::IsTrue(StartPosition::FromFen("bbqnnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRKR w KQkq - 0 1").has_value());
        Assert::IsTrue(StartPosition::FromFen("bbqnnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRKR w HFhf - 0 1").has_value()); // Shredder-FEN
        Assert::IsFalse(StartPosition::FromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1").has_value());
        Assert::IsFalse(StartPosition::FromFen("bbqnnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRKR b KQkq - 0 1").has_value());
        Assert::IsFalse(StartPosition::FromFen("bbqnnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRRK w KQkq - 0 1").has_value()); // not mirrored
        Assert::IsFalse(StartPosition::FromFen("bbqnnrrk/pppppppp/8/8/8/8/PPPPPPPP/BBQNNRRK w KQkq - 0 1").has_value()); // king not between
        Assert::IsFalse(StartPosition::FromFen("").has_value());
    }

    TEST_METHOD(Chess960_OutOfRange_Throws) {
        Assert::ExpectException<ChessException>([] { StartPosition::Chess960(-1); });
        Assert::ExpectException<ChessException>([] { StartPosition::Chess960(960); });
    }

    TEST_METHOD(Standard_IsNotChess960) {
        Assert::IsFalse(StartPosition::Standard().IsChess960());
        Assert::IsTrue(StartPosition::Chess960(518).IsChess960());
        Assert::IsTrue(StartPosition::Random().IsChess960());
        Assert::IsFalse(StartPosition::Standard() == StartPosition::Chess960(518));
    }
};

TEST_CLASS(Chess960Tests) {
public:
    // 60 random games checked against python-chess (scripts/make_chess960_reference.py): every move
    // is legal here, the number of legal moves agrees at every ply, the final FEN is the same, the
    // castlings' SAN too; then every move is taken back to the position before it.
    TEST_METHOD(ReferenceGames_MatchPythonChess) {
        for (const auto& reference : Reference960Games) {
            const auto start = StartPosition::Chess960(reference.Number);
            const auto moves = Words(reference.Moves);
            const auto counts = Words(reference.Counts);
            Assert::AreEqual(moves.size() + 1, counts.size());

            Game game;
            game.Restart(true, start);
            std::vector<std::string> fens{ game.MakeFen() };
            for (size_t ply = 0; ply <= moves.size(); ++ply) {
                const auto label = L"#" + std::to_wstring(reference.Number) + L" ply " + std::to_wstring(ply);
                Assert::AreEqual<size_t>(std::stoul(counts[ply]), LegalCount(game), label.c_str());
                if (ply == moves.size())
                    break;
                Move move = Move::Parse(moves[ply]);
                Assert::IsTrue(CanPlay(game, move.From, move.To), label.c_str());
                game.DoMove(move);
                fens.push_back(game.MakeFen());
            }
            const auto label = L"#" + std::to_wstring(reference.Number);
            Assert::AreEqual(std::string(reference.Fen), game.MakeFen(), label.c_str());

            const auto san = Game::FormatHistorySan(game.GetGameRecord(), start);
            for (const auto& castling : Words(reference.Castlings)) {
                const auto colon = castling.find(':');
                const auto ply = std::stoul(castling.substr(0, colon));
                Assert::IsTrue(game.GetGameRecord()[ply].IsCastlingMove(), label.c_str());
                Assert::AreEqual(castling.substr(colon + 1), san[ply], label.c_str());
            }

            for (size_t ply = moves.size(); ply > 0; --ply) {
                game.UndoMove();
                Assert::AreEqual(fens[ply - 1], game.MakeFen(), (label + L" takeback to ply " + std::to_wstring(ply - 1)).c_str());
            }
        }
    }

    // #3 BQNNRKRB: king f1, rook g1. O-O at once: the two swap.
    TEST_METHOD(KingAndRookSwap) {
        auto game = Play960(3, {});
        Assert::IsTrue(CanPlay(*game, f1, g1), L"O-O as the king onto its rook");
        game->DoMove(Move::Parse("f1g1"));

        Assert::IsTrue(game->GetPieceAt(g1) == WhiteKing);
        Assert::IsTrue(game->GetPieceAt(f1) == WhiteRook);
        const auto& castling = game->GetGameRecord().back();
        Assert::IsTrue(castling.IsCastlingMove());
        Assert::IsFalse(castling.IsCapturingMove(), L"its own rook isn't taken");
        Assert::AreEqual(std::string("f1g1"), castling.ToUciString());
        Assert::AreEqual(std::string("O-O"), Game::FormatHistorySan(game->GetGameRecord(), game->GetStartPosition())[0]);
        Assert::AreEqual(0, MaterialBalance(game->GetCurrentBoard()));

        game->UndoMove();
        Assert::AreEqual(StartPosition::Chess960(3).Fen(), game->MakeFen());
    }

    // #96 BBQNRNKR: king g1, rook h1. O-O moves only the rook (to f1) once the knight has left f1.
    TEST_METHOD(KingStaysWhereItIs) {
        auto game = Play960(96, { "f1e3", "f8e6" });
        game->DoMove(Move::Parse("g1h1"));
        Assert::IsTrue(game->GetPieceAt(g1) == WhiteKing);
        Assert::IsTrue(game->GetPieceAt(f1) == WhiteRook);
        Assert::IsTrue(game->GetPieceAt(h1) == Nothing);
        Assert::AreEqual(std::string("O-O"), Game::FormatHistorySan(game->GetGameRecord(), game->GetStartPosition())[2]);
        Assert::IsTrue(game->MakeFen().find(" b kq ") != std::string::npos, L"White can't castle any more");
    }

    // #746 RKNNBBQR: king b1, rook a1. O-O-O takes the king to c1 and the rook to d1: d1 must be free
    // too (the rook's square), not only the king's way.
    TEST_METHOD(QueensideFromB1) {
        auto game = Play960(746, { "c1b3", "c8b6" });
        Assert::IsFalse(CanPlay(*game, b1, a1), L"the knight on d1 blocks the rook's square");

        game->DoMove(Move::Parse("d1c3"));
        game->DoMove(Move::Parse("d8c6"));
        Assert::IsTrue(CanPlay(*game, b1, a1));
        game->DoMove(Move::Parse("b1a1"));
        Assert::IsTrue(game->GetPieceAt(c1) == WhiteKing);
        Assert::IsTrue(game->GetPieceAt(d1) == WhiteRook);
        Assert::IsTrue(game->GetPieceAt(a1) == Nothing);
        Assert::IsTrue(game->GetPieceAt(b1) == Nothing);
        Assert::AreEqual(std::string("O-O-O"), Game::FormatHistorySan(game->GetGameRecord(), game->GetStartPosition())[4]);
    }

    // #111 QNRNKRBB: the black bishop on d4 hits g1, where the king would land.
    TEST_METHOD(ThroughAnAttackedSquare) {
        auto game = Play960(111, Words("f2f4 g7g6 g1d4 h8g7 d4c5 g7d4"));
        Assert::IsFalse(game->IsInCheck());
        Assert::IsFalse(CanPlay(*game, e1, f1));
    }

    // #111: the kingside rook went from f1 to g1; the king onto it there isn't a castling.
    TEST_METHOD(RookMoved_NoCastling) {
        auto game = Play960(111, Words("f2f4 g7g6 g1d4 h8g7 d4c5 g7d4 c5d6 d4c3 b1c3 b8a6 f1g1 c7c6"));
        Assert::IsFalse(CanPlay(*game, e1, g1));
        Assert::IsTrue(game->MakeFen().find(" w Qkq ") != std::string::npos);
    }

    // #379 NRKRBNQB: king c1, rook b1, a black queen on a1. O-O-O leaves the king on c1 and takes the
    // rook to d1: the queen then sees the king. Only playing it with its rook finds that.
    TEST_METHOD(RookShieldedTheKing) {
        auto game = Play960(379, Words("b2b4 f8e6 e2e4 g7g6 g2g3 h8e5 h2h3 e5f4 g3g4 g8h8 a2a3 c7c5 f1h2 f4d2 d1d2 a8b6 d2d3 e6g5 b4b5 h8a1"));
        Assert::IsFalse(game->IsInCheck());
        Assert::IsFalse(CanPlay(*game, c1, b1));
    }

    // In a standard game castling stays e1g1, and the king onto its rook is no move; in Chess960 #518
    // (the same set-up) it is the other way round.
    TEST_METHOD(TheGameDecidesTheForm) {
        const auto opening = Words("e2e4 e7e5 g1f3 b8c6 f1c4 g8f6");

        Game standard;
        standard.Restart();
        for (const auto& m : opening)
            standard.DoMove(Move::Parse(m));
        Assert::IsTrue(CanPlay(standard, e1, g1));
        Assert::IsFalse(CanPlay(standard, e1, h1));

        auto chess960 = Play960(518, opening);
        Assert::IsTrue(CanPlay(*chess960, e1, h1));
        Assert::IsFalse(CanPlay(*chess960, e1, g1));
        chess960->DoMove(Move::Parse("e1h1"));
        Assert::IsTrue(chess960->GetPieceAt(g1) == WhiteKing);
        Assert::IsTrue(chess960->GetPieceAt(f1) == WhiteRook);
    }

    TEST_METHOD(LoadMoves_ReplaysFromTheStart) {
        Game game;
        game.Restart(true, StartPosition::Chess960(3));
        game.LoadMoves({ "f1g1", "f8g8", "e2e4" });
        Assert::AreEqual(StartPosition::Chess960(3).Fen(), game.MakeFen(), L"back at the start after loading");
        Assert::AreEqual(3, game.GetStartPosition().Number());

        game.ResumeFromLoad();
        Assert::AreEqual(3, game.GetMoveCount());
        Assert::IsTrue(game.GetPieceAt(g1) == WhiteKing);
    }

    TEST_METHOD(Restart_WithoutAStart_IsStandard) {
        auto game = Play960(3, { "f1g1" });
        game->Restart();
        Assert::IsFalse(game->GetStartPosition().IsChess960());
        Assert::AreEqual(StartPosition::Standard().Fen(), game->MakeFen());
    }

    // A premove of the king: the rook's square (the move as written) and the king's square.
    TEST_METHOD(PremoveTargets_RookAndKingSquare) {
        auto game = Play960(746, { "c1b3", "c8b6", "d1c3", "d8c6" });
        const auto targets = game->GetPremoveTargets(b1);
        Assert::IsTrue(std::find(targets.begin(), targets.end(), a1) != targets.end(), L"onto the rook");
        Assert::IsTrue(std::find(targets.begin(), targets.end(), c1) != targets.end(), L"the king's square");
    }
};

TEST_CLASS(Chess960BoardDiffTests) {
public:
    // The swap slides both pieces, nothing appears or disappears.
    TEST_METHOD(Swap_BothSlide) {
        const auto before = Play960(3, {})->GetCurrentBoard();
        const auto after = Play960(3, { "f1g1" })->GetCurrentBoard();
        BoardDiff diff(before, after);
        Assert::AreEqual<size_t>(2, diff.Moved().size());
        Assert::IsTrue(diff.Removed().empty() && diff.Added().empty());
        for (const auto& t : diff.Moved()) {
            if (t.Before() == WhiteKing)
                Assert::IsTrue(t.From() == f1 && t.To() == g1);
            else
                Assert::IsTrue(t.Before() == WhiteRook && t.From() == g1 && t.To() == f1);
        }
    }

    // The king stays: only the rook slides.
    TEST_METHOD(KingStays_OnlyTheRookSlides) {
        const auto before = Play960(96, { "f1e3", "f8e6" })->GetCurrentBoard();
        const auto after = Play960(96, { "f1e3", "f8e6", "g1h1" })->GetCurrentBoard();
        BoardDiff diff(before, after);
        Assert::AreEqual<size_t>(1, diff.Moved().size());
        Assert::IsTrue(diff.Moved()[0].Before() == WhiteRook && diff.Moved()[0].From() == h1 && diff.Moved()[0].To() == f1);
        Assert::IsTrue(diff.Removed().empty() && diff.Added().empty());
    }
};

}
