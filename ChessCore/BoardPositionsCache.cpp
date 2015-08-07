#include "stdafx.h"
#include "BoardPositionsCache.h"

using namespace Chess;

BoardPositionsCache::BoardPositionsCache(const BoardAptr &board)
	:_board(board)
{
}

void BoardPositionsCache::SetBoard(const BoardAptr &board)
{
	_board = board;
}

const BoardAptr &BoardPositionsCache::GetBoard() const
{
	return _board;
}

const BoardAttackMapAptr &BoardPositionsCache::GetAttackMap(EPieceColors side)
{
	assert(side != CEMPTY && "Non empty side expected only!");

	auto boardHash = _board->GetHashCode();
	auto currentHash = (side == LIGHT) ? &_lightAttackMap : &_darkAttackMap;

	auto foundPtr = currentHash->find(boardHash);
	if (foundPtr != currentHash->end())
	{
		return foundPtr->second;
	}

	auto bamPtr = std::make_shared<BoardAttackMap>();
	MoveGeneration::GetBoardAttackMap(*_board, *bamPtr, side);

	//(*currentHash)[boardHash] = bamPtr;
	currentHash->AddOrUpdate(boardHash, bamPtr);

	return (*currentHash)[boardHash];
}