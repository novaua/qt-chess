#pragma once
#include <QAbstractListModel>
#include <string>
#include <vector>

// The move list for QML: one row per move number, White's move and Black's. show() compares the
// new moves with the shown ones and changes only the rows that differ, so a new move adds one row
// (or fills in Black's half of the last one) instead of rebuilding the whole list (BL-14).
class MoveListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	// Ply = moves on the board after that move (for reviewTo); piece = "N"/"n" etc., or "" for pawns
	// and castling; text = the SAN without the piece letter. No black move yet: bPly 0, bText "".
	enum Roles {
		NumberRole = Qt::UserRole + 1, // n: the move number
		WhitePlyRole, WhitePieceRole, WhiteTextRole,
		BlackPlyRole, BlackPieceRole, BlackTextRole,
	};

	explicit MoveListModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = {}) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

	// Show these moves (SAN, from the first).
	void show(const std::vector<std::string>& san);

private:
	static int rowsFor(size_t plies) { return (int)(plies + 1) / 2; }
	QString piece(int ply) const; // of _san[ply - 1]
	QString text(int ply) const;

	std::vector<std::string> _san;
};
