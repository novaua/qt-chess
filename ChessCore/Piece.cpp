#include "stdafx.h"
#include "Piece.h"

using namespace Chess;
bool Piece::operator == (const Piece &o) const
{
	return o.Type == Type && o.Color == Color;
}
