#include "stdafx.h"
#include "Piece.h"

using namespace Chess;
bool Piece::operator == (const Piece &o) const
{
	return this == &o || o.Type == Type && o.Color == Color;
}

bool Piece::IsEmpty()
{
	return Color == CEMPTY;
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

	auto offset = Color == DARK ? 6 : 0;
	return translationMap[Type + offset];
}

size_t Piece::GetHashCode() const
{
	return Color == DARK ? EPC_MAX + Type: Type;
}