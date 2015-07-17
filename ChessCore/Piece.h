#pragma once
namespace Chess
{
	enum EPieceTypes
	{
		EMPTY,
		KNIGHT,
		BISHOP,
		ROOK,
		QUEEN,
		KING,
		PAWN,
	};

	enum EPieceColors
	{
		CEMPTY,
		LIGHT,
		DARK,
	};

	struct Piece
	{
		EPieceTypes Type;
		EPieceColors Color;
		bool operator == (const Piece &o)const;
	};
}
