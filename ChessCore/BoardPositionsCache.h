#pragma once
#include "Move.h"
#include "Board.h"
#include "LruCacheMap.hpp"

namespace Chess {

	// BoardHash -> Attack Map auto ptr 
	typedef LruCacheMap<size_t, BoardAttackMapAptr> BoardsAttackHash;

	class BoardPositionsCache
	{
	public:
		BoardPositionsCache(const BoardAptr &board);

		void SetBoard(const BoardAptr &board);
		const BoardAptr &GetBoard() const;

		const BoardAttackMapAptr &GetAttackMap(EPieceColors side);

	private:
		BoardsAttackHash _lightAttackMap;
		BoardsAttackHash _darkAttackMap;
		BoardAptr _board;
	};
}