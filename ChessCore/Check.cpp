#include "stdafx.h"
#include "Check.h"
#include "Move.h"
#include "BoardPositionsCache.h"

using namespace Chess;

GameChecks::GameChecks(const GameState &state)
	:_state(state)
{
}

GameChecks::~GameChecks()
{
}

PositionPiece GetKingPosition(const BoardAptr &board, EPieceColors side)
{
	return MoveGeneration::GetPositionsOf(*board, KING, side) [0];
}

bool GameChecks::IsInCheck(EPieceColors side)
{
	auto imTheKing = GetKingPosition(_state.Board, side);

	auto attackMap = _state.Cache->GetAttackMap(OtherSideOf(side));
	return MoveGeneration::IsUnderAttack(*attackMap, imTheKing.Position);
}

bool GameChecks::IsCheckMate(EPieceColors side)
{
	auto result = false;
	if (IsInCheck(side)) 
	{
		auto imTheKing = GetKingPosition(_state.Board, side);
		auto attackMap = _state.Cache->GetAttackMap(OtherSideOf(side));
		auto whoAttacksTheKing = attackMap->at(imTheKing.Position);
		
		// Can capture the checking piece
		auto kingAttackMoves = MoveGeneration::GenerateBasicMoves(*_state.Board, imTheKing.Position, side, true);
		
		if (!kingAttackMoves.empty())
		{
			std::vector<BoardPosition> kingAttackList;
			for (auto move : kingAttackMoves)
			{
				for (auto atacker : whoAttacksTheKing)
				{
					if (move.Capturing)
					{
						if (move.To == atacker.Position)
						{
							kingAttackList.push_back(move.To);
						}
					}
				}
			}

			auto newBoard = std::make_shared<Board>(*_state.Board);
			
			for (auto moveTo : kingAttackList)
			{
				newBoard->DoMove({ imTheKing.Position, moveTo });
				_state.Cache->SetBoard(newBoard);
				if (!IsInCheck(side))
				{
					_state.Cache->SetBoard(_state.Board);
					
					//King can capture the threatener
					return false;
				}
			}
		}
		else 
		{
			// Distant check
		}
	}

	return result;
}
