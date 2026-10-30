#include "stdafx.h"
#include "boardpieces.h"
#include "BoardDiff.h"

namespace {
	QString pieceText(const Chess::Piece& piece)
	{
		return QString::fromStdString(piece.ToString());
	}
}

BoardPieces::BoardPieces(QObject* parent)
	: QAbstractListModel(parent)
{}

int BoardPieces::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : (int)_rows.size();
}

QVariant BoardPieces::data(const QModelIndex& index, int role) const
{
	if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
		return {};
	const auto& row = _rows[index.row()];
	switch (role) {
	case PieceIdRole: return row.id;
	case SquareRole:  return row.square;
	case PieceRole:   return row.piece;
	case TakenRole:   return row.taken;
	default:          return {};
	}
}

QHash<int, QByteArray> BoardPieces::roleNames() const
{
	return {
		{ PieceIdRole, "pieceId" },
		{ SquareRole,  "square" },
		{ PieceRole,   "piece" },
		{ TakenRole,   "taken" },
	};
}

int BoardPieces::rowAt(int square) const
{
	for (int i = 0; i < (int)_rows.size(); ++i)
		if (_rows[i].square == square && !_rows[i].taken)
			return i;
	return -1;
}

int BoardPieces::pieceIdAt(int square) const
{
	const int row = rowAt(square);
	return row < 0 ? -1 : _rows[row].id;
}

void BoardPieces::reset(const Chess::Board& shown)
{
	beginResetModel();
	_rows.clear();
	for (int square = 0; square < Chess::BpMax; ++square) {
		const auto piece = shown.At(square);
		if (!piece.IsEmpty())
			_rows.push_back({ _nextId++, square, pieceText(piece), false });
	}
	_shown = shown;
	_hasShown = true;
	endResetModel();
}

void BoardPieces::sync(const Chess::Board& shown)
{
	if (!_hasShown) {
		reset(shown);
		return;
	}

	const Chess::BoardDiff diff(_shown, shown);
	if (diff.IsEmpty())
		return;
	if (diff.Size() > MaxChanges) {
		reset(shown);
		return;
	}

	// Find every row before changing any: a capture moves a piece onto the taken one's square.
	std::vector<std::pair<int, Chess::PieceTravel>> moved;
	for (const auto& travel : diff.Moved())
		moved.push_back({ rowAt(travel.From()), travel });
	std::vector<int> taken;
	for (const auto& removed : diff.Removed())
		taken.push_back(rowAt(removed.Position));
	const bool allFound = std::all_of(moved.begin(), moved.end(), [](const auto& m) { return m.first >= 0; })
		&& std::all_of(taken.begin(), taken.end(), [](int row) { return row >= 0; });
	if (!allFound) { // the rows don't match _shown (shouldn't happen): start over
		reset(shown);
		return;
	}

	// Rows of taken pieces stay (faded out) until the next reset, so a capture can fade even
	// when the robot answers at once.
	for (int row : taken) {
		_rows[row].taken = true;
		const auto index = this->index(row);
		emit dataChanged(index, index, { TakenRole });
	}
	for (const auto& [row, travel] : moved) {
		_rows[row].square = travel.To();
		_rows[row].piece = pieceText(travel.After());
		const auto index = this->index(row);
		emit dataChanged(index, index, { SquareRole, PieceRole });
	}
	if (!diff.Added().empty()) {
		const int first = (int)_rows.size();
		beginInsertRows({}, first, first + (int)diff.Added().size() - 1);
		for (const auto& added : diff.Added())
			_rows.push_back({ _nextId++, added.Position, pieceText(added.Piece), false });
		endInsertRows();
	}
	_shown = shown;
}
