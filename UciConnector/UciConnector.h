#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <random>
#include <vector>

class QProcess;

struct Command
{
	std::string Request;
	std::string Response;
};

struct StartPosMoveRequest
{
	std::string Fen;                 // empty → "position startpos"
	std::vector<std::string> Moves;  // moves appended after the position token
};

struct EngineMoveResponse
{
	std::string BestMove;
	std::string Ponder;
};

// One line of a MultiPV search: a root move and its score from the side to move's view.
struct EngineCandidate
{
	std::string Move;
	int ScoreCp = 0; // mate in N is reported as ±(MateScoreCp - N)
};

constexpr int MateScoreCp = 100000;

// Picks one of the candidates at random, weighting each by exp(-loss / temperatureCp), where
// loss is how many centipawns it is worse than the best one. A forced mate counts as only a
// fixed margin better than the best ordinary move, so a weak player can still miss it.
std::string PickWeakMove(const std::vector<EngineCandidate>& candidates, int temperatureCp,
                         std::mt19937& rng);

class UciConnector
{
	bool _initOk = false;
	std::unique_ptr<QProcess> _uciEngine;
	std::map<std::string, std::string> _opt;
	std::string _enginePath;
	std::deque<std::string> _traffic; // last lines sent (">>") and received ("<<"), for DebugState()

	// The engine process's own Win32 handle (PROCESS_TERMINATE), so Kill() can stop it from
	// another thread without touching QProcess, which belongs to the engine thread.
	std::mutex _processMutex;       // guards _processHandle
	void* _processHandle = nullptr; // nullptr when not open
	void OpenProcessHandle();       // after a successful start (engine thread)
	void CloseProcessHandle();      // called with the lock released for the OS call

	void Trace(const std::string& line);

protected:
	// Lines received before the expected response are appended to 'lines' when it is given.
	std::string ProcessCommand(const Command& comm, std::vector<std::string>* lines = nullptr);

public:
	UciConnector();
	// stockfish.exe paths Init() tries, in order (empty: no engine installed).
	static std::vector<std::string> EngineCandidates();
	void Init();
	bool IsInitialized();
	bool IsRunning() const; // false once the engine process has exited; Init() starts a fresh one
	bool CheckReady();
	bool NewGame();
	std::string GetOption(const std::string& op);
	void SetOption(const std::string& op, const std::string& value);
	std::vector<std::string> GetOptions();
	EngineMoveResponse GetEngineMove(const StartPosMoveRequest& req,
	                                 std::chrono::milliseconds moveTime);
	void SetDifficulty(int elo); // target Elo strength (clamped to Stockfish's supported 1320-3190 range)

	// The engine's 'count' best moves after a full-strength search to 'depth', best first.
	// For a finished game it returns the single "(none)" move, like GetEngineMove().
	std::vector<EngineCandidate> GetCandidateMoves(const StartPosMoveRequest& req, int count, int depth);
	void Kill();

	// Multi-line, human-readable engine state for bug reports: executable, process state /
	// exit code, stderr and the recent command traffic. Works in Release builds.
	std::string DebugState();
	~UciConnector();
};

typedef std::shared_ptr<UciConnector> UciConnectorAPtr;
