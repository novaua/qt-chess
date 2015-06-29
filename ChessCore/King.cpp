#include "stdafx.h"
#include "King.h"

using namespace Chess;


std::vector<int> GetValidOffsetForPoint(int x, int y)
{
	if (x == 0)
	{
		if (y == 0) 
		{
			return{ 8, 9, 1 };
		} 
		if (y == 8)
		{
			return{ -8, 9, -1 };
		}

	}
}

bool King::IsMoveValid(const Move &move, const Board &board)
{
	auto result = false;
	std::vector<int> ValidKingOffsets = {-7, -8, -9,
										 -1,      1,
										  7,  8,  9 };

	auto from = board[move.From->GetLocation()];
	auto to = board[move.To->GetLocation()];

	auto canMoveLeft = from->X() > 0;

	if (!canMoveLeft && offset == -1 || )

	for (const int &offset : ValidKingOffsets)
	{
		
		if (from->GetLocation() + offset == to->GetLocation())
		{
			return true;
		}
	}

	return result;
}

EPieceType GetType()
{
	return ept_king;
}