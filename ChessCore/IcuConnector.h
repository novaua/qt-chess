#pragma once
#include "Move.h"
namespace Chess
{
	class IcuConnector
	{
	public:
		IcuConnector(std::istream &istr, std::ostream &ostr);

		void Init();

		void StartPosition();

		bool IsReady();

		void PositionMoves(Move move);

		std::pair<Move, Move> Go();

		~IcuConnector();
	private:
		std::istream &_istr;
		std::ostream &_ostr;
	};
}

