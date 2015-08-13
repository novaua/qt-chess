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

	const int UniquePiecesCount = EPC_MAX - 1;

	EPieceColors OppositeSideOf(EPieceColors side);
	int GetPiceCount(EPieceTypes type);
	std::vector<size_t> MakeRandomVector(int elementsCount);

	struct Piece
	{
		EPieceTypes Type;
		EPieceColors Color;

		bool IsEmpty() const;
		bool operator == (const Piece &o)const;
		const std::string &ToString() const;
		size_t GetHashCode() const;
	};
}
