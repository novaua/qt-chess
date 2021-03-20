#include "stdafx.h"
#include "BoardPositionsCache.h"

using namespace Chess;

BoardPositionsCache::BoardPositionsCache()
{
}

const BoardAttackMapAptr &BoardPositionsCache::GetAttackMap(const BoardAptr &board, PieceColors side)
{
	assert(side != PieceColors::Empty && "Non empty side expected only!");

	auto boardHash = board->GetHashCode();
	auto currentHash = (side == PieceColors::Light) ? &_lightAttackMap : &_darkAttackMap;

	auto foundPtr = currentHash->find(boardHash);
	if (foundPtr != currentHash->end())
	{
		return foundPtr->second;
	}

	auto bamPtr = std::make_shared<BoardAttackMap>();
	MoveGeneration::GetBoardAttackMap(*board, *bamPtr, side);

	currentHash->AddOrUpdate(boardHash, bamPtr);

	return (*currentHash)[boardHash];
}

const BoardAttackMapAptr &BoardPositionsCache::GetViktimsMap(const BoardAptr &board, PieceColors side)
{
	assert(side != PieceColors::Empty && "Non empty side expected only!");

	auto boardHash = board->GetHashCode();
	auto currentHash = (side == PieceColors::Light) ? &_lightViktimMap : &_darkViktimMap;

	auto foundPtr = currentHash->find(boardHash);
	if (foundPtr != currentHash->end())
	{
		return foundPtr->second;
	}

	auto bamPtr = std::make_shared<BoardAttackMap>();
	MoveGeneration::GetBoardViktimsMap(*board, *bamPtr, side);

	currentHash->AddOrUpdate(boardHash, bamPtr);

	return (*currentHash)[boardHash];
}
