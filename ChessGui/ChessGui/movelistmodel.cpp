#include "stdafx.h"
#include "movelistmodel.h"

namespace {
	bool startsWithPiece(const std::string& san)
	{
		static const std::string letters = "NBRQK";
		return !san.empty() && letters.find(san[0]) != std::string::npos;
	}
}

MoveListModel::MoveListModel(QObject* parent)
	: QAbstractListModel(parent)
{}

int MoveListModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : rowsFor(_san.size());
}

QString MoveListModel::piece(int ply) const
{
	const auto& san = _san[ply - 1];
	if (!startsWithPiece(san))
		return {};
	const QString letter = QString(QChar(san[0]));
	return ply % 2 == 1 ? letter : letter.toLower();
}

QString MoveListModel::text(int ply) const
{
	const auto& san = _san[ply - 1];
	return QString::fromStdString(startsWithPiece(san) ? san.substr(1) : san);
}

QVariant MoveListModel::data(const QModelIndex& index, int role) const
{
	if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
		return {};
	const int whitePly = index.row() * 2 + 1;
	const int blackPly = whitePly + 1 <= (int)_san.size() ? whitePly + 1 : 0;
	switch (role) {
	case NumberRole:     return index.row() + 1;
	case WhitePlyRole:   return whitePly;
	case WhitePieceRole: return piece(whitePly);
	case WhiteTextRole:  return text(whitePly);
	case BlackPlyRole:   return blackPly;
	case BlackPieceRole: return blackPly ? piece(blackPly) : QString();
	case BlackTextRole:  return blackPly ? text(blackPly) : QString();
	default:             return {};
	}
}

QHash<int, QByteArray> MoveListModel::roleNames() const
{
	return {
		{ NumberRole,     "n" },
		{ WhitePlyRole,   "wPly" },
		{ WhitePieceRole, "wPiece" },
		{ WhiteTextRole,  "wText" },
		{ BlackPlyRole,   "bPly" },
		{ BlackPieceRole, "bPiece" },
		{ BlackTextRole,  "bText" },
	};
}

void MoveListModel::show(const std::vector<std::string>& san)
{
	// The first ply that differs; rows from its row on change.
	size_t same = 0;
	while (same < _san.size() && same < san.size() && _san[same] == san[same])
		++same;
	if (same == _san.size() && same == san.size())
		return;

	const int oldRows = rowsFor(_san.size());
	const int newRows = rowsFor(san.size());
	const int keptRows = std::min(oldRows, newRows);
	const int firstChanged = (int)same / 2;

	// The usual case — one move, or a few at once (Lichess) — adds to the end of the list: only the
	// new plies are copied. If the last old ply was White's, a new Black move fills its row, so that
	// row changes too; a new row comes after it when the list may go on to White's next move.
	if (same == _san.size()) {
		const bool lastRowChanged = same % 2 == 1;
		if (newRows > oldRows) {
			beginInsertRows({}, oldRows, newRows - 1);
			_san.insert(_san.end(), san.begin() + same, san.end());
			endInsertRows();
		}
		else {
			_san.insert(_san.end(), san.begin() + same, san.end());
		}
		if (lastRowChanged)
			emit dataChanged(index(firstChanged), index(firstChanged));
		return;
	}

	// In three steps, so each signal sees only its own change: rows that go, then the rows that
	// stay get their new moves, then new rows.
	if (newRows < oldRows) {
		beginRemoveRows({}, newRows, oldRows - 1);
		_san.resize(std::min(_san.size(), (size_t)newRows * 2));
		endRemoveRows();
	}
	const size_t keptPlies = std::min(san.size(), (size_t)keptRows * 2);
	_san.assign(san.begin(), san.begin() + keptPlies);
	if (firstChanged < keptRows)
		emit dataChanged(index(firstChanged), index(keptRows - 1));
	if (newRows > oldRows) {
		beginInsertRows({}, oldRows, newRows - 1);
		_san.insert(_san.end(), san.begin() + keptPlies, san.end());
		endInsertRows();
	}
}
