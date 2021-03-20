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
		BoardPositionsCache();

		// Me -> My killers list
		const BoardAttackMapAptr &GetAttackMap(const BoardAptr &board, PieceColors side);

		// Me -> My victims list
		const BoardAttackMapAptr &GetViktimsMap(const BoardAptr &board, PieceColors side);

	private:
		BoardsAttackHash _lightAttackMap;
		BoardsAttackHash _darkAttackMap;

		BoardsAttackHash _lightViktimMap;
		BoardsAttackHash _darkViktimMap;
	};
}