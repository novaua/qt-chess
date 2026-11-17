#pragma once
#include <array>
#include <optional>
#include <string>
#include "Piece.h"

namespace Chess
{
	// The pieces a game starts with (LI-01A): the standard set-up or one of Chess960's 960, numbered as
	// Scharnagl's scheme numbers them (the numbers lichess.org shows; #518 is the standard set-up).
	// White's back rank is BackRank(), Black's mirrors it, eight pawns on each side.
	//
	// IsChess960() belongs to the game, not to the set-up: #518 played as Chess960 castles as Chess960
	// does (the king onto its own rook), a standard game as standard chess does (e1g1).
	class StartPosition
	{
	public:
		static constexpr int StandardNumber = 518;
		static constexpr int Count = 960;

		StartPosition(); // the standard game

		static StartPosition Standard();
		static StartPosition Chess960(int number); // 0..959; throws ChessException outside
		static StartPosition Random();             // one of the 960
		// A Chess960 start position's FEN (Lichess's initialFen, a UCI "position fen"): the back ranks
		// mirrored, eight pawns each, nothing else, White to move. Empty when the FEN isn't one.
		static std::optional<StartPosition> FromFen(const std::string& fen);

		int  Number() const { return _number; }
		bool IsChess960() const { return _chess960; }
		const std::array<PieceTypes, 8>& BackRank() const { return _rank; }
		PieceTypes At(int file) const { return _rank[file]; }
		int  KingFile() const;
		int  RookFile(bool kingside) const; // the castling rook: right of the king (kingside) or left

		// The start FEN, castling "KQkq" (X-FEN: the outermost rooks, which at the start are the only ones).
		std::string Fen() const;

		bool operator==(const StartPosition&) const = default;

	private:
		StartPosition(const std::array<PieceTypes, 8>& rank, int number, bool chess960);
		static std::array<PieceTypes, 8> RankOf(int number);
		static int NumberOf(const std::array<PieceTypes, 8>& rank); // -1: not a Chess960 set-up

		std::array<PieceTypes, 8> _rank;
		int  _number;
		bool _chess960;
	};
}
