#include "stdafx.h"
#include <tuple>
#include "PieceMatch.h"

namespace Chess
{
	namespace
	{
		int FileOf(int square) { return square % 8; }
		int RankOf(int square) { return square / 8; }

		// Squared, so a knight's jump counts as nearer than a bishop's long diagonal.
		int Distance(int a, int b)
		{
			const int df = FileOf(a) - FileOf(b);
			const int dr = RankOf(a) - RankOf(b);
			return df * df + dr * dr;
		}

		int LastRank(PieceColors color) { return color == PieceColors::Light ? 7 : 0; }
		int RankBeforeLast(PieceColors color) { return color == PieceColors::Light ? 6 : 1; }

		// The pawn became the piece: a step or a capture onto the last rank, or the type changing
		// in place on it.
		bool IsPromotion(const PositionPiece& pawn, const PositionPiece& piece)
		{
			const auto color = pawn.Piece.Color;
			if (pawn.Piece.Type != PAWN || piece.Piece.Color != color
				|| piece.Piece.Type == PAWN || piece.Piece.Type == KING
				|| RankOf(piece.Position) != LastRank(color))
				return false;
			if (pawn.Position == piece.Position)
				return true;
			return RankOf(pawn.Position) == RankBeforeLast(color)
				&& std::abs(FileOf(pawn.Position) - FileOf(piece.Position)) <= 1;
		}

		// Pairs pieces that left a square with pieces that arrived on one, where `samePiece` allows
		// it, nearest first. Paired pieces are taken out of both lists.
		template <typename SamePiece>
		void Pair(std::vector<PositionPiece>& left, std::vector<PositionPiece>& arrived,
			SamePiece samePiece, std::vector<PieceTravel>& moved)
		{
			struct Candidate { int Distance; size_t Left, Arrived; };
			std::vector<Candidate> candidates;
			for (size_t l = 0; l < left.size(); ++l)
				for (size_t a = 0; a < arrived.size(); ++a)
					if (samePiece(left[l], arrived[a]))
						candidates.push_back({ Distance(left[l].Position, arrived[a].Position), l, a });

			std::sort(candidates.begin(), candidates.end(), [](const Candidate& x, const Candidate& y) {
				return std::tie(x.Distance, x.Left, x.Arrived) < std::tie(y.Distance, y.Left, y.Arrived);
			});

			std::vector<bool> leftUsed(left.size()), arrivedUsed(arrived.size());
			for (const auto& c : candidates)
			{
				if (leftUsed[c.Left] || arrivedUsed[c.Arrived])
					continue;
				leftUsed[c.Left] = arrivedUsed[c.Arrived] = true;
				const auto& from = left[c.Left];
				const auto& to = arrived[c.Arrived];
				moved.push_back({ from.Position, to.Position, from.Piece, to.Piece });
			}

			auto keepUnused = [](std::vector<PositionPiece>& pieces, const std::vector<bool>& used) {
				std::vector<PositionPiece> rest;
				for (size_t i = 0; i < pieces.size(); ++i)
					if (!used[i])
						rest.push_back(pieces[i]);
				pieces.swap(rest);
			};
			keepUnused(left, leftUsed);
			keepUnused(arrived, arrivedUsed);
		}
	}

	BoardDiff MatchPieces(const Board& before, const Board& after)
	{
		std::vector<PositionPiece> left, arrived;
		for (int i = 0; i < BpMax; ++i)
		{
			const auto was = before.At(i);
			const auto now = after.At(i);
			if (was == now)
				continue;
			if (!was.IsEmpty())
				left.push_back({ (BoardPosition)i, was });
			if (!now.IsEmpty())
				arrived.push_back({ (BoardPosition)i, now });
		}

		BoardDiff diff;
		Pair(left, arrived, [](const PositionPiece& l, const PositionPiece& a) {
			return l.Piece == a.Piece;
		}, diff.Moved);
		Pair(left, arrived, [](const PositionPiece& l, const PositionPiece& a) {
			return IsPromotion(l, a) || IsPromotion(a, l); // the promotion, or its takeback
		}, diff.Moved);

		diff.Removed = left;
		diff.Added = arrived;
		return diff;
	}
}
