#include "stdafx.h"
#include "San.h"
#include "MovesGen.h"

using namespace Chess;

namespace {
	bool isKingInCheck(const Board& board, PieceColors sideInCheck) {
		auto kings = MoveGeneration::GetPositionsOf(board, KING, sideInCheck);
		if (kings.empty()) return false;
		auto kingPos = kings[0].Position;
		auto attacker = OppositeSideOf(sideInCheck);
		bool inCheck = false;
		board.ForEachPiece([&](BoardPosition pos) {
			if (inCheck) return;
			for (const auto& mv : MoveGeneration::GenerateBasicMoves(board, pos, attacker, true))
				if (mv.To == kingPos) { inCheck = true; break; }
			}, attacker);
		return inCheck;
	}
}

std::string Chess::FormatMoveSan(const HistoryMove& m,
	const Board& boardAfter,
	bool isMate)
{
	auto fromPos = m.From.Position;
	auto toPos = m.To.Position;
	auto piece = m.From.Piece;
	const Board& boardBefore = *boardAfter.BeforeLastMove();

	if (m.IsCastlingMove())
		return ((int)toPos % 8 > (int)fromPos % 8) ? "O-O" : "O-O-O";

	std::string result;

	if (piece.Type == PAWN) {
		bool isCapture = m.IsCapturingMove() || m.IsEnPassantMove();
		result = isCapture
			? std::string(1, BoardPositionToString(fromPos)[0]) + "x" + BoardPositionToString(toPos)
			: BoardPositionToString(toPos);
		if (m.IsPawnPromotionMove()) {
			result += "=";
			result += m.PromotedTo.ToSANString();
		}
	}
	else {
		auto sym = piece.ToSANString();
		auto candidates = MoveGeneration::GetPositionsOf(boardBefore, piece.Type, piece.Color);
		bool needsRank = false, needsFile = false;
		for (const auto& pp : candidates) {
			if (pp.Position == fromPos) continue;
			for (const auto& mv : MoveGeneration::GenerateBasicMoves(
				boardBefore, pp.Position, piece.Color)) {
				if (mv.To == toPos) {
					if ((int)pp.Position % 8 == (int)fromPos % 8)
						needsRank = true;
					else
						needsFile = true;
				}
			}
		}
		std::string disambig;
		if (needsRank && needsFile)
			disambig = BoardPositionToString(fromPos);
		else if (needsRank)
			disambig = std::string(1, BoardPositionToString(fromPos)[1]);
		else if (needsFile)
			disambig = std::string(1, BoardPositionToString(fromPos)[0]);

		result = sym;
		result += disambig;
		result += (m.IsCapturingMove() ? "x" : "");
		result += BoardPositionToString(toPos);
	}

	auto oppColor = OppositeSideOf(piece.Color);
	bool isCheck = !isMate && isKingInCheck(boardAfter, oppColor);
	if (isMate)       result += "#";
	else if (isCheck) result += "+";
	return result;
}
