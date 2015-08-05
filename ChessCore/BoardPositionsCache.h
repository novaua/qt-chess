#pragma once
#include "Move.h"
#include "Board.h"

namespace Chess {

	// BoardHash -> Attack Map auto ptr 
	typedef std::map<size_t, BoardAttackMapAptr> BoardsAttackHash;

	class BoardPositionsCache
	{
	public:
		BoardPositionsCache(const BoardAptr &board);

		void SetBoard(const BoardAptr &board);

		const BoardAttackMapAptr &GetAttackMap(EPieceColors side);

	private:
		BoardsAttackHash _lightAttackMap;
		BoardsAttackHash _darkAttackMap;
		BoardAptr _board;
	};
}