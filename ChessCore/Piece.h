#pragma once
namespace Chess
{
	enum PieceTypes
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

	enum class PieceColors
	{
		Empty,
		Light,
		Dark,
	};

	const int UniquePiecesCount = EPC_MAX - 1;

	PieceColors OppositeSideOf(PieceColors side);
	int GetPiceCount(PieceTypes type);
	std::vector<size_t> MakeRandomVector(int elementsCount);

	struct Piece
	{
		PieceTypes Type;
		PieceColors Color;

		bool IsEmpty() const;
		bool operator == (const Piece &o)const;

		const std::string &ToString() const;
		static Piece Parse(const std::string &strPiece);

		size_t GetHashCode() const;
	};
}
