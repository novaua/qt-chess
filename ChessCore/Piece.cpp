#include "stdafx.h"
#include "Piece.h"

using namespace Chess;
bool Piece::operator == (const Piece &o) const
{
	return this == &o || o.Type == Type && o.Color == Color;
}

bool Piece::IsEmpty() const
{
	return Color == CEMPTY;
}

const int UniquePiecesCount = EPC_MAX - 1;

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

	auto offset = Color == DARK ? UniquePiecesCount : 0;
	return translationMap[Type + offset];
}



size_t Piece::GetHashCode() const
{
	static std::vector<size_t> hashVector = MakeRandomVector(UniquePiecesCount * 2);
	auto offset = Color == DARK ? UniquePiecesCount : 0;
	return Color == CEMPTY ? 0 : hashVector[Type + offset - 1];
}

namespace Chess
{
	EPieceColors OppositeSideOf(EPieceColors side)
	{
		assert(side != CEMPTY && "Empty Piece is not expected!");
		return side == LIGHT ? DARK : LIGHT;
	}

	int GetPiceCount(EPieceTypes type)
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
