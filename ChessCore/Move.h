#pragma once
#include "Board.h"

namespace Chess
{
	struct BinarySerializer
	{
		template<class T>
		static void Serialize(const T & plainObject, std::ostream &ostream)
		{
			ostream.write((char*)&plainObject, sizeof(T));
		}

		template<class T>
		static T Deserialize(std::istream &istream)
		{
			T out;
			istream.read((char*)&out, sizeof(T));
			return out;
		}
	};

	struct Move
	{
		BoardPosition From;

		BoardPosition To;

		bool Capturing;
	};

	struct PositionPiece
	{
		BoardPosition Position;
		Piece Piece;
	};

	struct HistoryMove
	{
		int Id;

		PositionPiece From;

		PositionPiece To;

		bool IsCapturingMove() const;
		Move ToMove() const;
	};
};