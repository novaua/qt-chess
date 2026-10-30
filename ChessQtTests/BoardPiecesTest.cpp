#include <QAbstractItemModelTester>
#include <CppUnitTest.h>
#include "Game.h"
#include "boardpieces.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// BoardPieces: a piece keeps its row and id while it moves; captured pieces are marked taken;
// a big change resets the model. QAbstractItemModelTester checks the model's signals; its
// complaints are warnings, collected here and expected to be none.
namespace BoardPiecesTests
{
	static QStringList g_modelWarnings;

	static void collectModelWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		if (type != QtDebugMsg && type != QtInfoMsg && QByteArray(context.category).startsWith("qt.modeltest"))
			g_modelWarnings << message;
	}

	// The position after the moves (UCI) from the start.
	static Chess::Board position(const std::vector<std::string>& moves)
	{
		Chess::Game game;
		game.Restart();
		for (const auto& m : moves)
			game.DoMove(Chess::Move::Parse(m));
		return game.GetCurrentBoard();
	}

	static std::vector<std::string> plus(std::vector<std::string> moves, const std::string& move)
	{
		moves.push_back(move);
		return moves;
	}

	struct Signals {
		int dataChanged = 0, inserted = 0, resets = 0;
	};

	static void count(BoardPieces& model, Signals& s)
	{
		QObject::connect(&model, &QAbstractItemModel::dataChanged, [&s] { ++s.dataChanged; });
		QObject::connect(&model, &QAbstractItemModel::rowsInserted, [&s] { ++s.inserted; });
		QObject::connect(&model, &QAbstractItemModel::modelReset, [&s] { ++s.resets; });
	}

	static int rowOfId(const BoardPieces& model, int id)
	{
		for (int r = 0; r < model.rowCount(); ++r)
			if (model.data(model.index(r), BoardPieces::PieceIdRole).toInt() == id)
				return r;
		return -1;
	}

	static QVariant roleOfId(const BoardPieces& model, int id, int role)
	{
		return model.data(model.index(rowOfId(model, id)), role);
	}

	TEST_CLASS(BoardPiecesTest)
	{
		QtMessageHandler _previousHandler = nullptr;

	public:
		TEST_METHOD_INITIALIZE(Init)
		{
			g_modelWarnings.clear();
			_previousHandler = qInstallMessageHandler(collectModelWarnings);
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			qInstallMessageHandler(_previousHandler);
		}

		void assertModelOk()
		{
			Assert::IsTrue(g_modelWarnings.isEmpty(), g_modelWarnings.join('\n').toStdWString().c_str());
		}

		TEST_METHOD(FirstSyncShowsEveryPiece)
		{
			BoardPieces model;
			QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
			model.sync(position({}));

			Assert::AreEqual(32, model.rowCount());
			Assert::AreEqual(QString("N"), roleOfId(model, model.pieceIdAt(Chess::g1), BoardPieces::PieceRole).toString());
			Assert::AreEqual(QString("k"), roleOfId(model, model.pieceIdAt(Chess::e8), BoardPieces::PieceRole).toString());
			Assert::AreEqual(-1, model.pieceIdAt(Chess::e4));
			assertModelOk();
		}

		TEST_METHOD(SamePositionChangesNothing)
		{
			BoardPieces model;
			model.sync(position({ "e2e4" }));
			Signals s;
			count(model, s);
			model.sync(position({ "e2e4" }));
			Assert::AreEqual(0, s.dataChanged + s.inserted + s.resets);
		}

		TEST_METHOD(MoveKeepsThePieceId)
		{
			BoardPieces model;
			QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
			model.sync(position({}));
			const int knight = model.pieceIdAt(Chess::g1);
			Signals s;
			count(model, s);

			model.sync(position({ "g1f3" }));

			Assert::AreEqual(knight, model.pieceIdAt(Chess::f3));
			Assert::AreEqual(-1, model.pieceIdAt(Chess::g1));
			Assert::AreEqual((int)Chess::f3, roleOfId(model, knight, BoardPieces::SquareRole).toInt());
			Assert::AreEqual(32, model.rowCount());
			Assert::AreEqual(1, s.dataChanged);
			Assert::AreEqual(0, s.inserted + s.resets);
			assertModelOk();
		}

		TEST_METHOD(CaptureMarksTheTakenPiece)
		{
			const std::vector<std::string> moves = { "e2e4", "d7d5" };
			BoardPieces model;
			QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
			model.sync(position(moves));
			const int whitePawn = model.pieceIdAt(Chess::e4);
			const int blackPawn = model.pieceIdAt(Chess::d5);

			model.sync(position(plus(moves, "e4d5")));

			Assert::AreEqual(whitePawn, model.pieceIdAt(Chess::d5));
			Assert::IsTrue(roleOfId(model, blackPawn, BoardPieces::TakenRole).toBool());
			Assert::IsFalse(roleOfId(model, whitePawn, BoardPieces::TakenRole).toBool());
			Assert::AreEqual(32, model.rowCount()); // the taken row stays, to fade out
			assertModelOk();
		}

		// The takeback brings the white pawn back with its id; the black pawn comes back as a new row.
		TEST_METHOD(TakebackOfACapture)
		{
			const std::vector<std::string> moves = { "e2e4", "d7d5" };
			BoardPieces model;
			QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
			model.sync(position(moves));
			const int whitePawn = model.pieceIdAt(Chess::e4);
			model.sync(position(plus(moves, "e4d5")));
			Signals s;
			count(model, s);

			model.sync(position(moves));

			Assert::AreEqual(whitePawn, model.pieceIdAt(Chess::e4));
			Assert::AreEqual(QString("p"), roleOfId(model, model.pieceIdAt(Chess::d5), BoardPieces::PieceRole).toString());
			Assert::AreEqual(1, s.inserted);
			Assert::AreEqual(33, model.rowCount());
			assertModelOk();
		}

		TEST_METHOD(CastlingMovesKingAndRook)
		{
			const std::vector<std::string> moves = { "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6" };
			BoardPieces model;
			model.sync(position(moves));
			const int king = model.pieceIdAt(Chess::e1);
			const int rook = model.pieceIdAt(Chess::h1);
			Signals s;
			count(model, s);

			model.sync(position(plus(moves, "e1g1")));

			Assert::AreEqual(king, model.pieceIdAt(Chess::g1));
			Assert::AreEqual(rook, model.pieceIdAt(Chess::f1));
			Assert::AreEqual(2, s.dataChanged);
			Assert::AreEqual(0, s.resets);
		}

		TEST_METHOD(PromotionKeepsThePawnsId)
		{
			const std::vector<std::string> moves = { "h2h4", "g7g5", "h4g5", "h7h6", "g5h6", "g8f6", "h6h7", "h8g8" };
			BoardPieces model;
			model.sync(position(moves));
			const int pawn = model.pieceIdAt(Chess::h7);

			model.sync(position(plus(moves, "h7h8q")));

			Assert::AreEqual(pawn, model.pieceIdAt(Chess::h8));
			Assert::AreEqual(QString("Q"), roleOfId(model, pawn, BoardPieces::PieceRole).toString());
		}

		// A new game after a middlegame: shown anew, with new ids and no taken rows left.
		TEST_METHOD(BigChangeResets)
		{
			const std::vector<std::string> middlegame = { "e2e4", "d7d5", "e4d5", "d8d5", "b1c3", "d5a5", "d2d4", "g8f6" };
			BoardPieces model;
			QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
			model.sync(position({ "e2e4", "d7d5" }));
			model.sync(position({ "e2e4", "d7d5", "e4d5" })); // a taken row
			model.sync(position(middlegame));
			Signals s;
			count(model, s);

			model.sync(position({}));

			Assert::AreEqual(1, s.resets);
			Assert::AreEqual(32, model.rowCount());
			for (int r = 0; r < model.rowCount(); ++r)
				Assert::IsFalse(model.data(model.index(r), BoardPieces::TakenRole).toBool());
			assertModelOk();
		}

		TEST_METHOD(RoleNamesForQml)
		{
			BoardPieces model;
			const auto names = model.roleNames();
			Assert::AreEqual(QByteArray("pieceId").toStdString(), names.value(BoardPieces::PieceIdRole).toStdString());
			Assert::AreEqual(QByteArray("square").toStdString(), names.value(BoardPieces::SquareRole).toStdString());
			Assert::AreEqual(QByteArray("piece").toStdString(), names.value(BoardPieces::PieceRole).toStdString());
			Assert::AreEqual(QByteArray("taken").toStdString(), names.value(BoardPieces::TakenRole).toStdString());
		}
	};
}
