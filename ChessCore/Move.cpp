#include "stdafx.h"
#include "Move.h"

using namespace Chess;

bool HistoryMove::IsCapturingMove() const
{
	return (To.Piece.Type != EMPTY);
}

Move HistoryMove::ToMove() const
{
	return{ From.Position, To.Position };
}