#pragma once
#include <vector>
#include <ostream>

namespace Chess {

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

	enum EPieceColors {
		EC_EMPTY,
		LIGHT,
		DARK
	};

	enum EMoves {
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8,
	};

	struct Piece
	{
		EPieceTypes Type;
		EPieceColors Color;
	};

	struct Move;

	class Board
	{
		std::vector<EPieceColors> _color;  /* EMPTY, LIGHT, DARK */
		std::vector<EPieceTypes> _piece;  /* PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, or EMPTY */
		bool _whiteFirst;
	public:

		Board(bool whiteFirst = true);
		void Initialize(bool whiteFirst);

		const std::vector<EPieceColors>  &color() const
		{
			return _color;
		}

		const std::vector<EPieceTypes> &piece() const
		{
			return _piece;
		}

		//Piece At(int position);

		bool AreWhiteFirst() const { return _whiteFirst; }

		Move DoMove(const Move &move, bool force = false);
		void ValidateMove(const Move &move);

		~Board();
	};

	struct Move
	{
		int From;

		int To;

		bool Capturing;

		Piece Captured;

		void Serialize(std::ostream &ostream) const;
		static Move Deserialize(std::istream &istream);
	};

	struct MoveGeneration
	{
		static std::vector<Move> GenerateMoves(const Board &board, int pieceOffset, EPieceColors side);
	};
};
