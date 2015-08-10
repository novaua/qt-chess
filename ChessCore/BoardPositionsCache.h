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

		// Me -> My killers list
		const BoardAttackMapAptr &GetAttackMap(EPieceColors side);

		// Me -> My victims list
		const BoardAttackMapAptr &GetViktimsMap(EPieceColors side);

	private:
		BoardsAttackHash _lightAttackMap;
		BoardsAttackHash _darkAttackMap;

		BoardsAttackHash _lightViktimMap;
		BoardsAttackHash _darkViktimMap;
		BoardAptr _board;
	};
}