#include "stdafx.h"
#include "Piece.h"

using namespace Chess;

bool Piece::IsMoveValid(const Move &move, const Board &board)
{
	return false;
}

EPieceType GetType()
{
	return ept_pnil;
}