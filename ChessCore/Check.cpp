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
	return MoveGeneration::GetPositionsOf(*board, KING, side)[0];
}

bool GameChecks::IsInCheck(EPieceColors side)
{
	auto imTheKing = GetKingPosition(_state.Board, side);

	auto attackMap = _state.Cache->GetAttackMap(OppositeSideOf(side));
	return MoveGeneration::IsUnderAttack(*attackMap, imTheKing.Position);
}

bool GameChecks::IsCheckMate(EPieceColors side)
{
	auto result = false;
	if (IsInCheck(side))
	{
		auto imTheKing = GetKingPosition(_state.Board, side);
		auto attackMap = _state.Cache->GetAttackMap(OppositeSideOf(side));

		auto whoAttacksTheKing = attackMap->at(imTheKing.Position);
		auto thisSideKillMap = _state.Cache->GetViktimsMap(side);

		// Can capture the checking piece or move away
		auto kingAttackMoves = MoveGeneration::GenerateBasicMoves(*_state.Board, imTheKing.Position, side);
		auto kingAttackList = (*thisSideKillMap)[imTheKing.Position];

		if (!kingAttackList.empty())
		{
			auto newBoard = std::make_shared<Board>(*_state.Board);

			for (auto moveTo : kingAttackList)
			{
				newBoard->DoMove({ imTheKing.Position, moveTo.Position });
				_state.Cache->SetBoard(newBoard);
				if (!IsInCheck(side))
				{
					_state.Cache->SetBoard(_state.Board);

					//King can capture the threatener or escape
					return false;
				}

				newBoard->UndoLastMove();
			}
		}

		auto newBoard = std::make_shared<Board>(*_state.Board);
		// Someone can protect King
		for (auto attacker : whoAttacksTheKing)
		{
			for (auto piecePtr = thisSideKillMap->begin(); piecePtr != thisSideKillMap->end(); ++piecePtr)
			{
				auto myPiece = _state.Board->At((BoardPosition)piecePtr->first);

				if (myPiece.Type == KING)
				{
					continue;
				}

				for (auto myPieceMovesTo : piecePtr->second)
				{
					if (attacker.Piece.Type == KNIGHT && myPieceMovesTo.Position != attacker.Position)
					{
						// my piece is unable to capture the Knight
						continue;
					}

					newBoard->DoMove({ (BoardPosition)piecePtr->first, myPieceMovesTo.Position });
					if (!IsInCheck(side))
					{
						_state.Cache->SetBoard(_state.Board);

						//Piece can capture the threatener or escape
						return false;
					}

					newBoard->UndoLastMove();
				}
			}
		}
	}

	return result;
}
