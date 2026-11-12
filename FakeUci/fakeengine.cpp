#include "stdafx.h"

#include "fakeengine.h"

#include "Board.h"
#include "BoardPositionsCache.h"
#include "Check.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

using namespace Chess;

namespace
{
	int SquareOf(const std::string& text, size_t at) // "e2" at `at` → 12, or -1
	{
		if (text.size() < at + 2)
			return -1;
		const int file = text[at] - 'a', rank = text[at + 1] - '1';
		return (0 <= file && file < 8 && 0 <= rank && rank < 8) ? rank * 8 + file : -1;
	}

	std::string SquareName(int square)
	{
		return { char('a' + square % 8), char('1' + square / 8) };
	}

	PieceTypes PieceOf(char c) // FEN / UCI letter, either case
	{
		switch (std::tolower(static_cast<unsigned char>(c))) {
		case 'p': return PAWN;
		case 'n': return KNIGHT;
		case 'b': return BISHOP;
		case 'r': return ROOK;
		case 'q': return QUEEN;
		case 'k': return KING;
		default:  return EMPTY;
		}
	}

	std::vector<std::string> Words(const std::string& line)
	{
		std::istringstream in(line);
		std::vector<std::string> words;
		for (std::string w; in >> w;)
			words.push_back(w);
		return words;
	}
}

FakeEngine::FakeEngine(std::vector<std::string> script, int delayMs)
	: _script(std::move(script)), _delayMs(delayMs)
{
	SetStartPosition();
}

void FakeEngine::SetStartPosition()
{
	auto board = std::make_shared<Board>();
	board->Initialize();
	_state = { board, std::make_shared<MovesHistory>(), std::make_shared<BoardPositionsCache>() };
	_side = PieceColors::Light;
	_ply = 0;
	_castling = "KQkq";
}

// Placement, side to move, castling, (en passant, half-move clock: not used), full-move number.
bool FakeEngine::SetFen(const std::string& fen)
{
	const auto fields = Words(fen);
	if (fields.size() < 2)
		return false;
	auto board = std::make_shared<Board>();
	board->Initialize();
	for (int square = 0; square < 64; ++square)
		board->Place(BoardPosition(square), Piece{ EMPTY, PieceColors::Empty });
	int rank = 7, file = 0;
	for (char c : fields[0]) {
		if (c == '/') { --rank; file = 0; }
		else if (std::isdigit(static_cast<unsigned char>(c))) file += c - '0';
		else {
			if (rank < 0 || file > 7 || PieceOf(c) == EMPTY)
				return false;
			const auto color = std::isupper(static_cast<unsigned char>(c)) ? PieceColors::Light : PieceColors::Dark;
			board->Place(BoardPosition(rank * 8 + file), Piece{ PieceOf(c), color });
			++file;
		}
	}
	_state = { board, std::make_shared<MovesHistory>(), std::make_shared<BoardPositionsCache>() };
	_side = fields[1] == "b" ? PieceColors::Dark : PieceColors::Light;
	_castling = fields.size() > 2 ? fields[2] : "-";

	// MoveGeneration makes castling and en passant only after a first move (an empty history is the
	// start position), and finds en passant in the last one: the other side's double step when the
	// FEN names its square, else an empty move (it moves no king or rook, so castling stays possible;
	// the FEN's castling field decides that, CastlingAllowed).
	const PieceColors other = OppositeSideOf(_side);
	const int ep = fields.size() > 3 ? SquareOf(fields[3], 0) : -1;
	HistoryMove last{};
	if (ep >= 0) {
		const int step = other == PieceColors::Light ? 8 : -8;
		last.From = { BoardPosition(ep - step), Piece{ PAWN, other } };
		last.To = { BoardPosition(ep + step), Piece{ EMPTY, PieceColors::Empty } };
	} else {
		last.From = { a1, Piece{ EMPTY, PieceColors::Empty } };
		last.To = { a1, Piece{ EMPTY, PieceColors::Empty } };
	}
	_state.History->push_back(last);
	const int fullMove = fields.size() > 5 ? std::max(1, std::atoi(fields[5].c_str())) : 1;
	_ply = 2 * (fullMove - 1) + (_side == PieceColors::Dark ? 1 : 0);
	return true;
}

// The move as Game::DoMove plays it, without the game's events: the rook of a castling or the pawn
// taken en passant, then the move itself, into the history (castling and en passant read it).
bool FakeEngine::Play(const std::string& uciMove)
{
	if (!IsLegal(uciMove))
		return false;
	Move move{ BoardPosition(SquareOf(uciMove, 0)), BoardPosition(SquareOf(uciMove, 2)), false,
	           Piece{ EMPTY, PieceColors::Empty }, false };
	if (uciMove.size() > 4)
		move.PromotedTo = Piece{ PieceOf(uciMove[4]), _side };
	MoveGeneration::Validate(_state, move, _side);
	Move complemental;
	if (MoveGeneration::AddComplementalMove(*_state.Board, move, complemental))
		_state.Board->DoMove(complemental);
	_state.History->push_back(_state.Board->DoMove(move));
	_side = OppositeSideOf(_side);
	++_ply;
	return true;
}

std::vector<std::string> FakeEngine::LegalMoves() const
{
	std::vector<std::string> moves;
	for (int square = 0; square < 64; ++square) {
		if (_state.Board->At(square).Color != _side)
			continue;
		auto generated = MoveGeneration::GenerateMoves(_state, BoardPosition(square), _side);
		MoveGeneration::ExcludeCheckMoves(_state, generated, _side);
		for (const auto& move : generated)
			if (CastlingAllowed(move))
				moves.push_back(ToUci(move));
	}
	std::sort(moves.begin(), moves.end());
	moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
	return moves;
}

// A FEN's castling field says what may still castle; the history (empty after a FEN) can't.
bool FakeEngine::CastlingAllowed(const Move& move) const
{
	const Piece piece = _state.Board->At(move.From);
	if (piece.Type != KING || std::abs(int(move.To) % 8 - int(move.From) % 8) != 2)
		return true;
	const bool kingSide = int(move.To) % 8 == 6;
	const char right = piece.Color == PieceColors::Light ? (kingSide ? 'K' : 'Q') : (kingSide ? 'k' : 'q');
	return _castling.find(right) != std::string::npos;
}

// A pawn to the last rank names its piece (a queen unless the move says otherwise), as the app's own
// moves do: without one it would be a promotion still waiting for its choice.
std::string FakeEngine::ToUci(const Move& move) const
{
	std::string text = SquareName(move.From) + SquareName(move.To);
	const int toRank = int(move.To) / 8;
	if (_state.Board->At(move.From).Type == PAWN && (toRank == 0 || toRank == 7)) {
		static const char letters[] = { ' ', 'n', 'b', 'r', 'q', 'k', 'p' };
		const PieceTypes promoted = move.PromotedTo.IsEmpty() ? QUEEN : move.PromotedTo.Type;
		text += letters[promoted];
	}
	return text;
}

// A legal from-to; a promotion to any piece (LegalMoves lists the queen only).
bool FakeEngine::IsLegal(const std::string& uciMove) const
{
	if (uciMove.size() < 4 || uciMove.size() > 5)
		return false;
	const auto legal = LegalMoves();
	return std::any_of(legal.begin(), legal.end(), [&](const std::string& m) {
		return m.compare(0, 4, uciMove, 0, 4) == 0 && (uciMove.size() == 4) == (m.size() == 4)
			&& (uciMove.size() == 4 || PieceOf(uciMove[4]) != EMPTY);
	});
}

std::string FakeEngine::ChooseMove() const
{
	if (_ply < int(_script.size())) {
		std::string scripted = _script[_ply];
		// A promotion written without its piece ("e7e8"): a queen, as Stockfish would choose.
		if (scripted.size() == 4 && !IsLegal(scripted) && IsLegal(scripted + 'q'))
			scripted += 'q';
		if (IsLegal(scripted))
			return scripted;
		// The player left the script, or the script is wrong: said, so a test that fails later shows why.
		std::cerr << "FakeUci: the script's move " << _script[_ply] << " at ply " << _ply
		          << " isn't legal there: the first legal move instead" << std::endl;
	}
	const auto legal = LegalMoves();
	return legal.empty() ? std::string() : legal.front();
}

void FakeEngine::OnPosition(const std::string& args)
{
	// "startpos [moves …]" or "fen <6 fields> [moves …]"
	const auto words = Words(args);
	size_t next = 0;
	if (!words.empty() && words[0] == "startpos") {
		SetStartPosition();
		next = 1;
	} else if (!words.empty() && words[0] == "fen") {
		std::string fen;
		for (next = 1; next < words.size() && words[next] != "moves"; ++next)
			fen += (fen.empty() ? "" : " ") + words[next];
		if (!SetFen(fen))
			SetStartPosition();
	}
	if (next < words.size() && words[next] == "moves")
		for (++next; next < words.size(); ++next)
			if (!Play(words[next]))
				break; // as Stockfish: the rest of an illegal list is ignored
}

void FakeEngine::OnGo(std::ostream& out)
{
	if (_delayMs > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(_delayMs));
	const std::string move = ChooseMove();
	if (move.empty()) {
		out << "bestmove (none)" << std::endl;
		return;
	}
	// One candidate, as GetCandidateMoves (MultiPV) reads it.
	out << "info depth 1 multipv 1 score cp 0 pv " << move << std::endl;
	out << "bestmove " << move << std::endl;
}

void FakeEngine::Run(std::istream& in, std::ostream& out)
{
	for (std::string line; std::getline(in, line);) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		const std::string command = line.substr(0, line.find(' '));
		const std::string args = line.size() > command.size() ? line.substr(command.size() + 1) : "";
		if (command == "uci") {
			out << "id name FakeUci (Chess++ tests)\n"
			       "id author Chess++\n"
			       "option name UCI_LimitStrength type check default false\n"
			       "option name UCI_Elo type spin default 1320 min 1320 max 3190\n"
			       "option name MultiPV type spin default 1 min 1 max 500\n"
			       "uciok" << std::endl;
		} else if (command == "isready") {
			out << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			SetStartPosition();
		} else if (command == "position") {
			OnPosition(args);
		} else if (command == "go") {
			OnGo(out);
		} else if (command == "moves") { // not UCI: the legal moves here, for looking into a test
			for (const auto& move : LegalMoves())
				out << move << ' ';
			out << "(ply " << _ply << ')' << std::endl;
		} else if (command == "quit") {
			return;
		}
		// setoption, stop, debug, …: nothing to do
	}
}
