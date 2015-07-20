#include "stdafx.h"
#include "Piece.h"

using namespace Chess;
bool Piece::operator == (const Piece &o) const
{
	return o.Type == Type && o.Color == Color;
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
