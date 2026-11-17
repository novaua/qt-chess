#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <CppUnitTest.h>
#define NOMINMAX
#include <windows.h>
#include "AppConfig.h"
#include "UserManager.h"
#include "gameanalysis.h"
#include "gameanalyzer.h"
#include "gamelibrary.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// BL-19 step 6: the engine's analysis of a kept game. GameAnalysis (win chances, judgements, accuracy,
// as lichess.org), its place on the record (GameLibrary), and GameAnalyzer on its thread, analysing with
// FakeUci (built next to this DLL): its score is the material after its move, or a mate.
namespace GameAnalysisTests
{
	using J = GameAnalysis::Judgement;

	// The scholar's mate: 3… Nf6?? lets 4. Qxf7#.
	static const QStringList ScholarsMate = { "e2e4", "e7e5", "d1h5", "b8c6", "f1c4", "g8f6", "h5f7" };

	TEST_CLASS(GameAnalysisTest)
	{
	public:
		TEST_METHOD(WinPercent_EvenAtZero_MateAsTheCeiling)
		{
			Assert::AreEqual(50.0, GameAnalysis::winPercent(0), 1e-9);
			Assert::AreEqual(100.0 - GameAnalysis::winPercent(-300), GameAnalysis::winPercent(300), 1e-9);
			Assert::IsTrue(GameAnalysis::winPercent(300) > 70 && GameAnalysis::winPercent(300) < 80);
			Assert::AreEqual(GameAnalysis::winPercent(1000), GameAnalysis::winPercent(GameAnalysis::MateCp - 3), 1e-9);
			Assert::AreEqual(GameAnalysis::winPercent(-1000), GameAnalysis::winPercent(-GameAnalysis::MateCp), 1e-9);
		}

		TEST_METHOD(MateIn_FromTheScore)
		{
			Assert::AreEqual(3, GameAnalysis::mateIn(GameAnalysis::MateCp - 3));
			Assert::AreEqual(-1, GameAnalysis::mateIn(-GameAnalysis::MateCp + 1));
			Assert::AreEqual(0, GameAnalysis::mateIn(GameAnalysis::MateCp)); // mated on the board: mate in 0
			Assert::IsTrue(GameAnalysis::isMate(GameAnalysis::MateCp));
			Assert::AreEqual(0, GameAnalysis::mateIn(950));
		}

		TEST_METHOD(MoveAccuracy_FullWithoutADrop)
		{
			Assert::AreEqual(100.0, GameAnalysis::moveAccuracy(40, 45), 1e-9);
			const double fivePoints = GameAnalysis::moveAccuracy(50, 45), thirtyPoints = GameAnalysis::moveAccuracy(50, 20);
			Assert::IsTrue(fivePoints > 75 && fivePoints < 85, L"a 5-point drop");
			Assert::IsTrue(thirtyPoints > 20 && thirtyPoints < 35, L"a 30-point drop");
			Assert::AreEqual(0.0, GameAnalysis::moveAccuracy(100, 0), 1e-9);
		}

		// Positions: 0 → -60 White's inaccuracy (5.5 points), -60 → -200 White's mistake (12),
		// -200 → +300 Black's blunder (43).
		TEST_METHOD(Judgements_ByTheMoversDrop)
		{
			const GameAnalysis a(14, { 0, -60, -60, -200, 300, 300, 300 }, {});
			Assert::AreEqual(6, a.plyCount());
			Assert::IsTrue(a.judgement(0) == J::Inaccuracy, L"White 0 → -60");
			Assert::IsTrue(a.judgement(1) == J::None);
			Assert::IsTrue(a.judgement(2) == J::Mistake, L"White -60 → -200");
			Assert::IsTrue(a.judgement(3) == J::Blunder, L"Black -200 → +300");
			Assert::IsTrue(a.judgement(4) == J::None);
			Assert::AreEqual(1, a.count(true, J::Inaccuracy));
			Assert::AreEqual(1, a.count(true, J::Mistake));
			Assert::AreEqual(0, a.count(true, J::Blunder));
			Assert::AreEqual(1, a.count(false, J::Blunder));
			Assert::AreEqual(0, a.count(false, J::Inaccuracy));
			Assert::AreEqual(QStringLiteral("blunder"), GameAnalysis::judgementName(a.judgement(3)));
			Assert::IsTrue(a.drop(3) > 40, L"the mover's drop, from Black's view");
		}

		TEST_METHOD(Accuracy_PerfectPlayIsFull_ABlunderPullsItDown)
		{
			const GameAnalysis even(14, { 20, 20, 20, 20, 20 }, {});
			Assert::AreEqual(100.0, even.accuracy(true), 1e-6);
			Assert::AreEqual(100.0, even.accuracy(false), 1e-6);

			const GameAnalysis a(14, { 0, -60, -60, -200, 300, 300, 300 }, {});
			const double white = a.accuracy(true), black = a.accuracy(false);
			Assert::IsTrue(white > 60 && white < 100, L"an inaccuracy and a mistake");
			Assert::IsTrue(black > 0 && black < white, L"one blunder in three moves");

			const GameAnalysis oneMove(14, { 0, -60 }, {});
			Assert::AreEqual(-1.0, oneMove.accuracy(false), 1e-9, L"Black made no move");
		}

		TEST_METHOD(Json_RoundTrip_AndWhatIsntOne)
		{
			const GameAnalysis a(18, { 20, 35, GameAnalysis::MateCp - 1, GameAnalysis::MateCp }, { "e2e4", "e7e5", "d1h5", "" },
			                     { "e2e4", "e7e5", "d1h5" });
			const GameAnalysis back = GameAnalysis::fromJson(a.toJson());
			Assert::IsTrue(back == a);
			Assert::AreEqual(18, back.depth());
			Assert::AreEqual(QStringLiteral("d1h5"), back.bestMove(2));
			Assert::AreEqual(QString(), back.bestMove(3));
			Assert::IsTrue(back.fits({ "e2e4", "e7e5", "d1h5" }));
			Assert::IsFalse(back.fits({ "e2e4", "e7e5" }));
			Assert::IsFalse(back.fits({ "d2d4", "d7d5", "c2c4" }), L"as many moves, other ones");
			QJsonObject noKey = a.toJson();
			noKey.remove(QStringLiteral("moves"));
			Assert::IsFalse(GameAnalysis::fromJson(noKey).fits({ "e2e4", "e7e5", "d1h5" }), L"of no moves: fits none");

			QJsonObject later = a.toJson();
			later[QStringLiteral("version")] = GameAnalysis::FormatVersion + 1;
			Assert::IsTrue(GameAnalysis::fromJson(later).isEmpty(), L"a later format isn't read");
			Assert::IsTrue(GameAnalysis::fromJson({}).isEmpty());
			Assert::IsTrue(GameAnalysis().toJson().isEmpty());
		}
	};

	// The folder of this test DLL: FakeUci.exe is built next to it.
	static QString testDllFolder()
	{
		HMODULE module = nullptr;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&testDllFolder), &module);
		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(module, path, MAX_PATH);
		return QFileInfo(QString::fromWCharArray(path)).absolutePath();
	}

	TEST_CLASS(GameAnalyzerTest)
	{
		std::unique_ptr<QTemporaryDir> _dir;
		std::unique_ptr<UserManager> _users;
		std::unique_ptr<GameLibrary> _library;
		std::unique_ptr<GameAnalyzer> _analyzer;

		QString keep(const QStringList& moves, bool finished = true)
		{
			GameRecord r;
			r.kind = QStringLiteral("local");
			r.opponentName = QStringLiteral("Guest");
			r.setMoves(moves);
			r.finished = finished;
			if (finished) {
				r.result = QStringLiteral("1-0");
				r.resultText = QStringLiteral("White wins by checkmate");
			}
			return _library->put(r);
		}

		GameAnalysis analysisOf(const QString& id) const
		{
			return GameAnalysis::fromJson(_library->find(id)->analysis);
		}

	public:
		TEST_METHOD_INITIALIZE(SetUp)
		{
			_dir = std::make_unique<QTemporaryDir>();
			AppConfig::setAppDataDir(_dir->path());
			_users = std::make_unique<UserManager>();
			_users->createUser(QStringLiteral("Alice"), QStringLiteral("wizard"));
			_library = std::make_unique<GameLibrary>();
			_library->setUserManager(_users.get());
			_analyzer = std::make_unique<GameAnalyzer>();
			_analyzer->setLibrary(_library.get());
			_analyzer->setPositionMode(Chess::PositionMode::StartPos);
			const QString fake = QDir(testDllFolder()).filePath(QStringLiteral("FakeUci.exe"));
			Assert::IsTrue(QFileInfo::exists(fake), L"FakeUci.exe is built next to the tests");
			qputenv("CHESS_ENGINE_PATH", QDir::toNativeSeparators(fake).toUtf8());
			qputenv("FAKEUCI_MOVES", ScholarsMate.join(' ').toUtf8()); // it sees the game's moves: Qxf7 mates
			qunsetenv("FAKEUCI_DELAY_MS");
		}

		TEST_METHOD_CLEANUP(TearDown)
		{
			_analyzer.reset(); // waits for its thread: before the QCoreApplication goes
			_library.reset();
			_users.reset();
			qunsetenv("CHESS_ENGINE_PATH");
			qunsetenv("FAKEUCI_MOVES");
			qunsetenv("FAKEUCI_DELAY_MS");
			AppConfig::setAppDataDir({});
			_dir.reset();
		}

		TEST_METHOD(AGame_EveryPositionScored_KeptOnItsRecord)
		{
			const QString id = keep(ScholarsMate);
			QSignalSpy finished(_analyzer.get(), &GameAnalyzer::finished);
			QSignalSpy failed(_analyzer.get(), &GameAnalyzer::failed);
			_analyzer->setDepth(10);
			Assert::IsTrue(_analyzer->start(id));
			Assert::IsTrue(_analyzer->running());
			Assert::AreEqual(8, _analyzer->total(), L"the start and 7 positions after a move");
			Assert::IsTrue(finished.wait(20000), L"analysed");
			Assert::AreEqual(0, (int)failed.count());
			Assert::IsFalse(_analyzer->running());
			Assert::AreEqual(id, finished.at(0).at(0).toString());

			const GameAnalysis a = analysisOf(id);
			Assert::AreEqual(7, a.plyCount());
			Assert::AreEqual(10, a.depth());
			Assert::AreEqual(QStringLiteral("e2e4"), a.bestMove(0), L"FakeUci follows the game");
			Assert::AreEqual(QStringLiteral("h5f7"), a.bestMove(6));
			Assert::AreEqual(GameAnalysis::MateCp - 1, a.eval(6), L"White mates in 1 there");
			Assert::AreEqual(GameAnalysis::MateCp, a.eval(7), L"Black is mated: not asked of the engine");
			Assert::AreEqual(QString(), a.bestMove(7));
			Assert::IsTrue(a.judgement(5) == J::Blunder, L"3… Nf6?? lets the mate in");
			Assert::AreEqual(1, a.count(false, J::Blunder));
			Assert::AreEqual(0, a.count(true, J::Blunder));
			Assert::IsTrue(_library->find(id)->toVariant().value(QStringLiteral("analysed")).toBool());

			QCoreApplication::processEvents(); // written
			GameLibrary again;
			again.setUserManager(_users.get());
			Assert::IsTrue(GameAnalysis::fromJson(again.find(id)->analysis) == a, L"kept in games.json");
		}

		// LI-01A: the analysis tells the engine a Chess960 game's start. FakeUci follows the scholar's mate
		// script where it can: after White's O-O (f1g1) its move for Black is 1… e5, so it played the castling.
		TEST_METHOD(Chess960_TheEngineIsToldTheStart)
		{
			Chess::Game game;
			game.Restart(true, Chess::StartPosition::Chess960(3));
			game.DoMove(Chess::Move::Parse("f1g1"));
			AnalysisWorker worker;
			QSignalSpy done(&worker, &AnalysisWorker::positionDone);
			QSignalSpy finished(&worker, &AnalysisWorker::finished);
			worker.analyse(1, game.GetGameRecord(), 6, Chess::PositionMode::FenWindow, false, 0, game.GetStartPosition());
			Assert::AreEqual(1, (int)finished.count());
			Assert::AreEqual(2, (int)done.count());
			Assert::AreEqual(QStringLiteral("e2e4"), done.at(0).at(3).toString());
			Assert::AreEqual(QStringLiteral("e7e5"), done.at(1).at(3).toString(), L"Black's move: the castling was played");
		}

		// Stockfish (./engines/ or PATH, as the other engine tests): its scores are the side to move's,
		// turned to White's here.
		TEST_METHOD(RealEngine_TheBlunderAndTheMate)
		{
			qunsetenv("CHESS_ENGINE_PATH");
			const QString id = keep(ScholarsMate);
			QSignalSpy finished(_analyzer.get(), &GameAnalyzer::finished);
			_analyzer->setDepth(10);
			Assert::IsTrue(_analyzer->start(id));
			Assert::IsTrue(finished.wait(60000), L"analysed by Stockfish");
			const GameAnalysis a = analysisOf(id);
			Assert::IsTrue(std::abs(a.eval(0)) < 100, L"the start is about even");
			Assert::AreEqual(QStringLiteral("h5f7"), a.bestMove(6), L"Stockfish sees the mate");
			Assert::AreEqual(1, GameAnalysis::mateIn(a.eval(6)), L"White mates in 1");
			Assert::AreEqual(GameAnalysis::MateCp, a.eval(7));
			Assert::IsTrue(a.judgement(5) == J::Blunder, L"3… Nf6??");
			Assert::IsTrue(a.accuracy(false) < a.accuracy(true));
		}

		TEST_METHOD(Cancel_NothingKept)
		{
			qputenv("FAKEUCI_DELAY_MS", "300");
			const QString id = keep(ScholarsMate);
			QSignalSpy finished(_analyzer.get(), &GameAnalyzer::finished);
			QSignalSpy failed(_analyzer.get(), &GameAnalyzer::failed);
			QSignalSpy running(_analyzer.get(), &GameAnalyzer::runningChanged);
			Assert::IsTrue(_analyzer->start(id));
			QTest::qWait(400);
			_analyzer->cancel();
			Assert::IsFalse(_analyzer->running());
			Assert::AreEqual(2, (int)running.count(), L"started, stopped");
			QTest::qWait(500);
			Assert::AreEqual(0, (int)finished.count());
			Assert::AreEqual(0, (int)failed.count(), L"a cancel isn't a failure");
			Assert::IsTrue(_library->find(id)->analysis.isEmpty());
		}

		// The player changed the game (a takeback) or deleted it meanwhile: it stops at once, without a
		// failure to report (review of #167-#170).
		TEST_METHOD(TheGameChangesMeanwhile_StoppedQuietly_NothingKept)
		{
			qputenv("FAKEUCI_DELAY_MS", "200");
			const QString id = keep(ScholarsMate.mid(0, 4), false);
			QSignalSpy failed(_analyzer.get(), &GameAnalyzer::failed);
			Assert::IsTrue(_analyzer->start(id));
			auto record = *_library->find(id);
			record.setMoves(ScholarsMate.mid(0, 3)); // a takeback
			_library->put(record);
			Assert::IsFalse(_analyzer->running(), L"at once");
			QTest::qWait(300);
			Assert::AreEqual(0, (int)failed.count());
			Assert::IsTrue(_library->find(id)->analysis.isEmpty());

			Assert::IsTrue(_analyzer->start(id));
			Assert::IsTrue(_library->remove(id));
			Assert::IsFalse(_analyzer->running(), L"deleted");
			QTest::qWait(300);
			Assert::AreEqual(0, (int)failed.count());
		}

		// One at a time: another game's start stops the one running, and says so.
		TEST_METHOD(AnotherGame_InterruptsTheRunningOne)
		{
			qputenv("FAKEUCI_DELAY_MS", "200");
			const QString first = keep(ScholarsMate);
			const QString second = keep(ScholarsMate.mid(0, 4), false);
			QSignalSpy interrupted(_analyzer.get(), &GameAnalyzer::interrupted);
			Assert::IsTrue(_analyzer->start(first));
			Assert::IsTrue(_analyzer->start(first), L"the same game again");
			Assert::AreEqual(0, (int)interrupted.count(), L"not another game's");
			Assert::IsTrue(_analyzer->start(second));
			Assert::AreEqual(1, (int)interrupted.count());
			Assert::AreEqual(first, interrupted.at(0).at(0).toString());
			Assert::AreEqual(second, _analyzer->gameId());
			_analyzer->cancel();
		}

		TEST_METHOD(NothingToAnalyse_NotStarted)
		{
			Assert::IsFalse(_analyzer->start(QStringLiteral("no-such-game")));
			GameRecord empty;
			empty.kind = QStringLiteral("local");
			Assert::IsFalse(_analyzer->start(_library->put(empty)), L"no move");
			GameRecord damaged;
			damaged.kind = QStringLiteral("local");
			damaged.moves = QStringList{ "e2e4", "e2e4" };
			Assert::IsFalse(_analyzer->start(_library->put(damaged)), L"a move that can't be played");
			Assert::IsFalse(_analyzer->running());
		}

		// The record's side of it (GameLibrary): kept only for the moves it was made for, and dropped
		// when the moves change (a takeback, the game went on).
		TEST_METHOD(Library_AnalysisOnlyForItsMoves)
		{
			const QString id = keep(ScholarsMate.mid(0, 2), false);
			const GameAnalysis a(10, { 20, 30, 20 }, { "e2e4", "e7e5", "g1f3" }, ScholarsMate.mid(0, 2));
			Assert::IsFalse(_library->setAnalysis(id, ScholarsMate.mid(0, 3), a), L"other moves");
			Assert::IsFalse(_library->setAnalysis(QStringLiteral("nope"), ScholarsMate.mid(0, 2), a));
			Assert::IsTrue(_library->setAnalysis(id, ScholarsMate.mid(0, 2), a));
			Assert::IsTrue(analysisOf(id) == a);

			auto record = *_library->find(id);
			record.whiteMs = 1000; // put again, the same moves (leaving the game): it stays
			_library->put(record);
			Assert::IsTrue(analysisOf(id) == a);

			record.setMoves({ "d2d4", "d7d5" }); // other moves, as many (typed in over it)
			_library->put(record);
			Assert::IsTrue(_library->find(id)->analysis.isEmpty(), L"dropped: not of these moves");

			record.setMoves(ScholarsMate.mid(0, 2));
			_library->put(record);
			Assert::IsTrue(_library->setAnalysis(id, ScholarsMate.mid(0, 2), a));
			record = *_library->find(id);
			record.setMoves(ScholarsMate.mid(0, 1)); // a takeback
			_library->put(record);
			Assert::IsTrue(_library->find(id)->analysis.isEmpty());
		}
	};
}
