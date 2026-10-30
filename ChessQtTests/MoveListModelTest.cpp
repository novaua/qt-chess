#include <QAbstractItemModelTester>
#include <CppUnitTest.h>
#include "movelistmodel.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft::VisualStudio::CppUnitTestFramework {
	template<>
	inline std::wstring ToString(const QString& q) { return q.toStdWString(); }
}

// MoveListModel: a new move changes one row (a new row for White, the second half of the last row
// for Black), a takeback removes or clears only what it takes back, never a reset. As in
// BoardPiecesTest, QAbstractItemModelTester's complaints are collected and expected to be none.
namespace MoveListModelTests
{
	static QStringList g_modelWarnings;

	static void collectModelWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message)
	{
		if (type != QtDebugMsg && type != QtInfoMsg && QByteArray(context.category).startsWith("qt.modeltest"))
			g_modelWarnings << message;
	}

	struct Signals {
		int dataChanged = 0, inserted = 0, removed = 0, resets = 0;
	};

	static void count(MoveListModel& model, Signals& s)
	{
		QObject::connect(&model, &QAbstractItemModel::dataChanged, [&s] { ++s.dataChanged; });
		QObject::connect(&model, &QAbstractItemModel::rowsInserted, [&s] { ++s.inserted; });
		QObject::connect(&model, &QAbstractItemModel::rowsRemoved, [&s] { ++s.removed; });
		QObject::connect(&model, &QAbstractItemModel::modelReset, [&s] { ++s.resets; });
	}

	static QVariant role(const MoveListModel& model, int row, int role)
	{
		return model.data(model.index(row), role);
	}

	TEST_CLASS(MoveListModelTest)
	{
		QtMessageHandler _previousHandler = nullptr;
		MoveListModel _model;
		std::unique_ptr<QAbstractItemModelTester> _tester;
		Signals _signals;

		// Show these moves, then count the signals of what follows.
		void start(const std::vector<std::string>& san)
		{
			_model.show(san);
			count(_model, _signals);
		}

		void assertModelOk()
		{
			Assert::IsTrue(g_modelWarnings.isEmpty(), g_modelWarnings.join('\n').toStdWString().c_str());
			Assert::AreEqual(0, _signals.resets);
		}

	public:
		TEST_METHOD_INITIALIZE(Init)
		{
			g_modelWarnings.clear();
			_previousHandler = qInstallMessageHandler(collectModelWarnings);
			_tester = std::make_unique<QAbstractItemModelTester>(&_model, QAbstractItemModelTester::FailureReportingMode::Warning);
		}

		TEST_METHOD_CLEANUP(Cleanup)
		{
			_tester.reset();
			qInstallMessageHandler(_previousHandler);
		}

		TEST_METHOD(OneMove)
		{
			_model.show({ "e4" });

			Assert::AreEqual(1, _model.rowCount());
			Assert::AreEqual(1, role(_model, 0, MoveListModel::NumberRole).toInt());
			Assert::AreEqual(1, role(_model, 0, MoveListModel::WhitePlyRole).toInt());
			Assert::AreEqual(QString(), role(_model, 0, MoveListModel::WhitePieceRole).toString());
			Assert::AreEqual(QString("e4"), role(_model, 0, MoveListModel::WhiteTextRole).toString());
			Assert::AreEqual(0, role(_model, 0, MoveListModel::BlackPlyRole).toInt());
			Assert::AreEqual(QString(), role(_model, 0, MoveListModel::BlackTextRole).toString());
			assertModelOk();
		}

		// The piece letter becomes the image key, in the mover's colour; castling stays text.
		TEST_METHOD(PieceLettersAndCastling)
		{
			_model.show({ "Nf3", "Nc6", "O-O" });

			Assert::AreEqual(QString("N"), role(_model, 0, MoveListModel::WhitePieceRole).toString());
			Assert::AreEqual(QString("f3"), role(_model, 0, MoveListModel::WhiteTextRole).toString());
			Assert::AreEqual(QString("n"), role(_model, 0, MoveListModel::BlackPieceRole).toString());
			Assert::AreEqual(QString("c6"), role(_model, 0, MoveListModel::BlackTextRole).toString());
			Assert::AreEqual(2, role(_model, 0, MoveListModel::BlackPlyRole).toInt());
			Assert::AreEqual(QString(), role(_model, 1, MoveListModel::WhitePieceRole).toString());
			Assert::AreEqual(QString("O-O"), role(_model, 1, MoveListModel::WhiteTextRole).toString());
			Assert::AreEqual(2, role(_model, 1, MoveListModel::NumberRole).toInt());
			assertModelOk();
		}

		TEST_METHOD(BlackMoveFillsTheLastRow)
		{
			start({ "e4" });
			_model.show({ "e4", "e5" });

			Assert::AreEqual(1, _model.rowCount());
			Assert::AreEqual(QString("e5"), role(_model, 0, MoveListModel::BlackTextRole).toString());
			Assert::AreEqual(1, _signals.dataChanged);
			Assert::AreEqual(0, _signals.inserted + _signals.removed);
			assertModelOk();
		}

		TEST_METHOD(WhiteMoveAddsARow)
		{
			start({ "e4", "e5" });
			_model.show({ "e4", "e5", "Nf3" });

			Assert::AreEqual(2, _model.rowCount());
			Assert::AreEqual(1, _signals.inserted);
			Assert::AreEqual(0, _signals.dataChanged + _signals.removed);
			assertModelOk();
		}

		// Black's answer and White's next move at once (Lichess): the last row, then a new one.
		TEST_METHOD(TwoMovesAtOnce)
		{
			start({ "e4" });
			_model.show({ "e4", "e5", "Nf3" });

			Assert::AreEqual(2, _model.rowCount());
			Assert::AreEqual(QString("e5"), role(_model, 0, MoveListModel::BlackTextRole).toString());
			Assert::AreEqual(1, _signals.dataChanged);
			Assert::AreEqual(1, _signals.inserted);
			assertModelOk();
		}

		// A takeback vs the robot: the player's move and its answer.
		TEST_METHOD(TakebackOfTwoMovesRemovesARow)
		{
			start({ "e4", "e5", "Nf3", "Nc6" });
			_model.show({ "e4", "e5" });

			Assert::AreEqual(1, _model.rowCount());
			Assert::AreEqual(1, _signals.removed);
			Assert::AreEqual(0, _signals.dataChanged + _signals.inserted);
			assertModelOk();
		}

		TEST_METHOD(TakebackOfBlacksMoveClearsItsHalf)
		{
			start({ "e4", "e5" });
			_model.show({ "e4" });

			Assert::AreEqual(1, _model.rowCount());
			Assert::AreEqual(0, role(_model, 0, MoveListModel::BlackPlyRole).toInt());
			Assert::AreEqual(QString(), role(_model, 0, MoveListModel::BlackTextRole).toString());
			Assert::AreEqual(1, _signals.dataChanged);
			Assert::AreEqual(0, _signals.inserted + _signals.removed);
			assertModelOk();
		}

		TEST_METHOD(SameMovesChangeNothing)
		{
			start({ "e4", "e5" });
			_model.show({ "e4", "e5" });

			Assert::AreEqual(0, _signals.dataChanged + _signals.inserted + _signals.removed);
		}

		TEST_METHOD(NewGameEmptiesTheList)
		{
			start({ "e4", "e5", "Nf3" });
			_model.show({});

			Assert::AreEqual(0, _model.rowCount());
			Assert::AreEqual(1, _signals.removed);
			assertModelOk();
		}

		// Another game of the same length (a loaded game): the changed rows are read again.
		TEST_METHOD(OtherMovesSameLength)
		{
			start({ "e4", "e5", "Nf3" });
			_model.show({ "d4", "d5", "c4" });

			Assert::AreEqual(QString("d4"), role(_model, 0, MoveListModel::WhiteTextRole).toString());
			Assert::AreEqual(QString("c4"), role(_model, 1, MoveListModel::WhiteTextRole).toString());
			Assert::AreEqual(1, _signals.dataChanged);
			Assert::AreEqual(0, _signals.inserted + _signals.removed);
			assertModelOk();
		}

		TEST_METHOD(RoleNames)
		{
			const auto names = _model.roleNames();
			for (const char* name : { "n", "wPly", "wPiece", "wText", "bPly", "bPiece", "bText" })
				Assert::IsTrue(names.values().contains(QByteArray(name)), QString(name).toStdWString().c_str());
		}
	};
}
