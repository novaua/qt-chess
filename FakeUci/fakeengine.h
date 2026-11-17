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
//    move N when that is legal there (a promotion without its piece: a queen). A test writes the
//    line it expects; the robot plays its half.
//  - otherwise the first legal move in a fixed order (by the UCI text); a scripted move it can't
//    play is said on stderr.
// FAKEUCI_DELAY_MS: it "thinks" that long before it answers (a test can leave while it thinks).
// Its one line's score (BL-19 step 6, the analysis) is "mate 1" when its move mates, else the material
// after its move from the side to move's view (pawn 100, knight and bishop 300, rook 500, queen 900):
// an analysis sees a piece taken or left to be taken, and a mate, the same way every time.
// Chess960 (LI-01A) as Stockfish plays it: "setoption name UCI_Chess960 value true", then "position fen"
// of a Chess960 start and its moves; castling is the king onto its own rook.
class FakeEngine
{
public:
	FakeEngine(std::vector<std::string> script, int delayMs);

	// Reads commands until "quit" or the end of the input.
	void Run(std::istream& in, std::ostream& out);

	// The position (for the tests of the engine itself, and the "position" command)
	void SetStartPosition(const Chess::StartPosition& start = Chess::StartPosition());
	bool SetFen(const std::string& fen);   // false: not a FEN it understands
	void SetChess960(bool on) { _chess960 = on; } // UCI_Chess960: a start FEN is then a Chess960 game's
	bool Play(const std::string& uciMove); // false: not legal here (the position is unchanged)
	std::vector<std::string> LegalMoves() const; // in UCI, sorted
	std::string ChooseMove() const;              // "" when there is none (mate, stalemate)
	std::string ScoreOf(const std::string& uciMove) const; // the UCI score of a legal move here: "cp 100", "mate 1"
	int Material() const;                        // the material balance, the side to move's view
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
	bool _chess960 = false;
};
