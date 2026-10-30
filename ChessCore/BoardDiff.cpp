#include "stdafx.h"
#include <tuple>
#include "BoardDiff.h"

namespace Chess
{
	PieceTravel::PieceTravel(const PositionPiece& from, const PositionPiece& to)
		: _from(from), _to(to)
	{}

	BoardDiff::BoardDiff(const Board& before, const Board& after)
	{
		// Every changed square: what left it goes to _removed, what arrived to _added ...
		for (int i = 0; i < BpMax; ++i)
		{
			const auto was = before.At(i);
			const auto now = after.At(i);
			if (was == now)
				continue;
			if (!was.IsEmpty())
				_removed.push_back({ (BoardPosition)i, was });
			if (!now.IsEmpty())
				_added.push_back({ (BoardPosition)i, now });
		}

		// ... then the pieces found on both sides move to _moved.
		Pair(IsSameKind);
		Pair(IsPromotionOrItsTakeback);
	}

	void BoardDiff::Pair(const SamePiece& samePiece)
	{
		struct Candidate { int Distance; size_t Left, Arrived; };
		std::vector<Candidate> candidates;
		for (size_t l = 0; l < _removed.size(); ++l)
			for (size_t a = 0; a < _added.size(); ++a)
				if (samePiece(_removed[l], _added[a]))
					candidates.push_back({ Distance(_removed[l].Position, _added[a].Position), l, a });

		std::sort(candidates.begin(), candidates.end(), [](const Candidate& x, const Candidate& y) {
			return std::tie(x.Distance, x.Left, x.Arrived) < std::tie(y.Distance, y.Left, y.Arrived);
		});

		std::vector<bool> leftPaired(_removed.size()), arrivedPaired(_added.size());
		for (const auto& c : candidates)
		{
			if (leftPaired[c.Left] || arrivedPaired[c.Arrived])
				continue;
			leftPaired[c.Left] = arrivedPaired[c.Arrived] = true;
			_moved.emplace_back(_removed[c.Left], _added[c.Arrived]);
		}

		auto keepUnpaired = [](std::vector<PositionPiece>& pieces, const std::vector<bool>& paired) {
			std::vector<PositionPiece> rest;
			for (size_t i = 0; i < pieces.size(); ++i)
				if (!paired[i])
					rest.push_back(pieces[i]);
			pieces.swap(rest);
		};
		keepUnpaired(_removed, leftPaired);
		keepUnpaired(_added, arrivedPaired);
	}

	bool BoardDiff::IsSameKind(const PositionPiece& left, const PositionPiece& arrived)
	{
		return left.Piece == arrived.Piece;
	}

	bool BoardDiff::IsPromotion(const PositionPiece& pawn, const PositionPiece& piece)
	{
		const auto color = pawn.Piece.Color;
		const int lastRank = color == PieceColors::Light ? 7 : 0;
		const int rankBeforeLast = color == PieceColors::Light ? 6 : 1;
		if (pawn.Piece.Type != PAWN || piece.Piece.Color != color
			|| piece.Piece.Type == PAWN || piece.Piece.Type == KING
			|| RankOf(piece.Position) != lastRank)
			return false;
		if (pawn.Position == piece.Position)
			return true;
		return RankOf(pawn.Position) == rankBeforeLast
			&& std::abs(FileOf(pawn.Position) - FileOf(piece.Position)) <= 1;
	}

	bool BoardDiff::IsPromotionOrItsTakeback(const PositionPiece& left, const PositionPiece& arrived)
	{
		return IsPromotion(left, arrived) || IsPromotion(arrived, left);
	}

	// Squared, so a knight's jump counts as nearer than a bishop's long diagonal.
	int BoardDiff::Distance(BoardPosition a, BoardPosition b)
	{
		const int df = FileOf(a) - FileOf(b);
		const int dr = RankOf(a) - RankOf(b);
		return df * df + dr * dr;
	}
}
