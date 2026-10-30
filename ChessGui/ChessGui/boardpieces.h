#pragma once
#include <QAbstractListModel>
#include <QList>
#include "Board.h"

// The pieces on the board, for QML: one row per piece. A piece keeps its row and id while it
// moves (and when it promotes), so the board can slide it from square to square instead of
// drawing the position anew (BL-04, docs/BL-04-PieceMovePlan.md).
class BoardPieces : public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles {
		PieceIdRole = Qt::UserRole + 1, // stable while the piece is on the board
		SquareRole,                     // 0 = a1 ... 63 = h8
		PieceRole,                      // "N" a white knight, "n" a black one
		TakenRole,                      // captured: the board fades it out
	};

	// More changes than this at once (a new game, a jump in the review) and the position is shown
	// anew, without slides. One move is at most 2 (castling, a capture).
	static constexpr size_t MaxChanges = 4;

	explicit BoardPieces(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

	// Show this position: the pieces that moved change their square, captured ones are marked
	// taken, new ones are added; or, if much changed, the model is reset.
	void sync(const Chess::Board& shown);

	// The id of the piece on that square (not taken), or -1.
	int pieceIdAt(int square) const;

private:
	struct Row {
		int id;
		int square;
		QString piece;
		bool taken;
	};

	void reset(const Chess::Board& shown);
	int rowAt(int square) const; // the row of the piece on that square (not taken), or -1

	QList<Row> _rows;
	Chess::Board _shown;
	bool _hasShown = false;
	int _nextId = 0;
};
