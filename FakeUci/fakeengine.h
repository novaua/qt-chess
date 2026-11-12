#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "MovesGen.h"

// A stand-in UCI engine for the UI tests (BL-05 step 3, docs/UiTestSuitePlan.md §4): what the app
// asks of Stockfish (uci, isready, ucinewgame, setoption, position, go, quit), answered at once and
// the same way every time. Its moves:
//  - FAKEUCI_MOVES, a whole game in UCI ("e2e4 e7e5 g1f3 …"): asked at ply N, it plays the game's
//    move N when that is legal there. A test writes the line it expects; the robot plays its half.
//  - otherwise the first legal move in a fixed order (by the UCI text).
// FAKEUCI_DELAY_MS: it "thinks" that long before it answers (a test can leave while it thinks).
class FakeEngine
{
public:
	FakeEngine(std::vector<std::string> script, int delayMs);

	// Reads commands until "quit" or the end of the input.
	void Run(std::istream& in, std::ostream& out);

	// The position (for the tests of the engine itself, and the "position" command)
	void SetStartPosition();
	bool SetFen(const std::string& fen);   // false: not a FEN it understands
	bool Play(const std::string& uciMove); // false: not legal here (the position is unchanged)
	std::vector<std::string> LegalMoves() const; // in UCI, sorted
	std::string ChooseMove() const;              // "" when there is none (mate, stalemate)
	bool IsLegal(const std::string& uciMove) const;
	int Ply() const { return _ply; }

private:
	void OnPosition(const std::string& args);
	void OnGo(std::ostream& out);
	bool CastlingAllowed(const Chess::Move& move) const;
	std::string ToUci(const Chess::Move& move) const;

	std::vector<std::string> _script;
	int _delayMs;
	Chess::GameState _state;
	Chess::PieceColors _side = Chess::PieceColors::Light;
	int _ply = 0;
	std::string _castling = "KQkq"; // from the FEN: what may still castle
};
