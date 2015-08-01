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
		EPC_MAX,
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

		bool IsEmpty();
		bool operator == (const Piece &o)const;
		const std::string &ToString() const;
		size_t GetHashCode() const;
	};
}
