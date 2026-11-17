#include "stdafx.h"
#include <cstring>
#include "StartPosition.h"
#include "ChessException.h"

namespace Chess
{
	namespace
	{
		const char* const Letters = " NBRQKP"; // by PieceTypes, White's (upper case)

		char LetterOf(PieceTypes type, bool white)
		{
			const char c = Letters[type];
			return white ? c : (char)std::tolower((unsigned char)c);
		}

		// The five knight placements on the five squares the bishops and the queen leave, as
		// Scharnagl's scheme orders them.
		const int KnightPairs[10][2] = {
			{ 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 2, 3 }, { 2, 4 }, { 3, 4 }
		};

		std::vector<std::string> Split(const std::string& text, char delimiter)
		{
			std::vector<std::string> parts;
			std::string part;
			std::istringstream in(text);
			while (std::getline(in, part, delimiter))
				if (delimiter != ' ' || !part.empty())
					parts.push_back(part);
			return parts;
		}
	}

	StartPosition::StartPosition()
		: StartPosition(RankOf(StandardNumber), StandardNumber, false)
	{}

	StartPosition::StartPosition(const std::array<PieceTypes, 8>& rank, int number, bool chess960)
		: _rank(rank), _number(number), _chess960(chess960)
	{}

	StartPosition StartPosition::Standard()
	{
		return StartPosition();
	}

	StartPosition StartPosition::Chess960(int number)
	{
		if (number < 0 || number >= Count)
			throw ChessException("No Chess960 position " + std::to_string(number));
		return StartPosition(RankOf(number), number, true);
	}

	StartPosition StartPosition::Random()
	{
		static std::mt19937 rng{ std::random_device{}() };
		return Chess960(std::uniform_int_distribution<int>(0, Count - 1)(rng));
	}

	// Scharnagl: n % 4 places the light-squared bishop (b, d, f, h), then % 4 the dark-squared one
	// (a, c, e, g), % 6 the queen on the free squares, the rest (0..9) the two knights; R K R fill the
	// three squares left.
	std::array<PieceTypes, 8> StartPosition::RankOf(int number)
	{
		std::array<PieceTypes, 8> rank;
		rank.fill(EMPTY);
		auto freeSquares = [&rank]() {
			std::vector<int> free;
			for (int f = 0; f < 8; ++f)
				if (rank[f] == EMPTY)
					free.push_back(f);
			return free;
		};

		int n = number;
		rank[2 * (n % 4) + 1] = BISHOP; n /= 4;
		rank[2 * (n % 4)] = BISHOP;     n /= 4;
		rank[freeSquares()[n % 6]] = QUEEN; n /= 6;
		const auto forKnights = freeSquares();
		rank[forKnights[KnightPairs[n][0]]] = KNIGHT;
		rank[forKnights[KnightPairs[n][1]]] = KNIGHT;
		const auto rest = freeSquares();
		rank[rest[0]] = ROOK;
		rank[rest[1]] = KING;
		rank[rest[2]] = ROOK;
		return rank;
	}

	int StartPosition::NumberOf(const std::array<PieceTypes, 8>& rank)
	{
		int lightBishop = -1, darkBishop = -1, queen = -1;
		std::vector<int> knights, rooksAndKing;
		std::vector<PieceTypes> afterBishops;
		for (int f = 0; f < 8; ++f) {
			switch (rank[f]) {
			case BISHOP:
				if (f % 2 == 1) { if (lightBishop >= 0) return -1; lightBishop = f / 2; }
				else            { if (darkBishop >= 0) return -1;  darkBishop = f / 2; }
				break;
			case QUEEN:
			case KNIGHT:
			case ROOK:
			case KING:
				afterBishops.push_back(rank[f]);
				break;
			default:
				return -1;
			}
		}
		if (lightBishop < 0 || darkBishop < 0 || afterBishops.size() != 6)
			return -1;

		std::vector<PieceTypes> afterQueen;
		for (int i = 0; i < 6; ++i) {
			if (afterBishops[i] == QUEEN) {
				if (queen >= 0) return -1;
				queen = i;
			} else {
				afterQueen.push_back(afterBishops[i]);
			}
		}
		if (queen < 0)
			return -1;

		for (int i = 0; i < 5; ++i)
			if (afterQueen[i] == KNIGHT)
				knights.push_back(i);
			else
				rooksAndKing.push_back(afterQueen[i]);
		if (knights.size() != 2 || rooksAndKing.size() != 3
			|| rooksAndKing[0] != ROOK || rooksAndKing[1] != KING || rooksAndKing[2] != ROOK)
			return -1;

		int pair = -1;
		for (int i = 0; i < 10; ++i)
			if (KnightPairs[i][0] == knights[0] && KnightPairs[i][1] == knights[1])
				pair = i;
		return ((pair * 6 + queen) * 4 + darkBishop) * 4 + lightBishop;
	}

	std::optional<StartPosition> StartPosition::FromFen(const std::string& fen)
	{
		const auto fields = Split(fen, ' ');
		if (fields.size() < 2 || fields[1] != "w")
			return std::nullopt;
		const auto ranks = Split(fields[0], '/');
		if (ranks.size() != 8 || ranks[1] != "pppppppp" || ranks[6] != "PPPPPPPP")
			return std::nullopt;
		for (int r = 2; r < 6; ++r)
			if (ranks[r] != "8")
				return std::nullopt;
		if (ranks[0].size() != 8 || ranks[7].size() != 8)
			return std::nullopt;

		std::array<PieceTypes, 8> rank;
		for (int f = 0; f < 8; ++f) {
			const char* found = std::strchr(Letters + 1, ranks[7][f]);
			if (!found || *found == 'P' || std::tolower((unsigned char)ranks[7][f]) != ranks[0][f])
				return std::nullopt;
			rank[f] = (PieceTypes)(found - Letters);
		}
		const int number = NumberOf(rank);
		if (number < 0)
			return std::nullopt;
		return StartPosition(rank, number, true);
	}

	int StartPosition::KingFile() const
	{
		for (int f = 0; f < 8; ++f)
			if (_rank[f] == KING)
				return f;
		return -1;
	}

	int StartPosition::RookFile(bool kingside) const
	{
		const int king = KingFile();
		if (kingside) {
			for (int f = king + 1; f < 8; ++f)
				if (_rank[f] == ROOK)
					return f;
		} else {
			for (int f = king - 1; f >= 0; --f)
				if (_rank[f] == ROOK)
					return f;
		}
		return -1;
	}

	std::string StartPosition::Fen() const
	{
		std::string white, black;
		for (auto type : _rank) {
			white += LetterOf(type, true);
			black += LetterOf(type, false);
		}
		return black + "/pppppppp/8/8/8/8/PPPPPPPP/" + white + " w KQkq - 0 1";
	}
}
