#include "stdafx.h"
#include "Piece.h"

using namespace Chess;
bool Piece::operator == (const Piece &o) const
{
	return this == &o || (o.Type == Type && o.Color == Color);
}

bool Piece::IsEmpty() const
{
	return Color == PieceColors::Empty;
}

const std::string &Piece::ToString() const
{
	static std::vector<std::string> translationMap = {
		" ",
		"n",
		"b",
		"r",
		"q",
		"k",
		"p",
		"N",
		"B",
		"R",
		"Q",
		"K",
		"P",
	};

	auto offset = Color == PieceColors::Light ? UniquePiecesCount : 0;
	return translationMap[Type + offset];
}

Piece Piece::Parse(const std::string &strPiece)
{
	// this is not absolutely efficient but quite pretty =)
	if (strPiece != " " && strPiece != "")
	{
		for (int t = 0; t < EPC_MAX; ++t)
		{
			for (int c = 1; c <= 2; ++c)
			{
				Piece p = { (PieceTypes)t, (PieceColors)c };
				if (p.ToString() == strPiece)
				{
					return p;
				}
			}
		}
	}

	return{ EMPTY, PieceColors::Empty };
}

size_t Piece::GetHashCode() const
{
	auto offset = Color == PieceColors::Dark ? UniquePiecesCount : 0;
	auto hash = Color == PieceColors::Empty ? 0 : Type + offset;
	return hash;
}

namespace Chess
{
	PieceColors OppositeSideOf(PieceColors side)
	{
		assert(side != PieceColors::Empty && "Empty Piece is not expected!");
		return side == PieceColors::Light ? PieceColors::Dark : PieceColors::Light;
	}

	int GetPiceCount(PieceTypes type)
	{
		static int counts[] = { 0, 2, 2, 2, 1, 1, 8 };
		return counts[type];
	}

	std::vector<size_t> MakeRandomVector(int elementsCount)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<size_t> dist(1, std::numeric_limits<size_t>::max());
		std::vector<size_t> result(elementsCount);
		std::set<size_t> was;
		for (int i = 0; i < elementsCount; ++i)
		{
			size_t value;
			do {
				value = dist(rd);
			} while (was.find(value) != was.end());

			result[i] = value;
			was.insert(value);
		}

		return result;
	}
}
