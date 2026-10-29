// UciConnector.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "UciConnector.h"
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>
#include <format>

const QString UciEngineProgrammExe = "stockfish.exe";

const std::string UciInitCommand = "uci";
const std::string UciOkCommand = "uciok";
const std::string UciNewGameCommand = "ucinewgame";
const std::string ReadyCommand = "isready";
const std::string ReadyOkCommand = "readyok";
const std::string BestMoveCommand = "bestmove";
const std::string QuitCommand = "quit";
const std::string LimitStrengthOption = "UCI_LimitStrength";
const std::string EloOption = "UCI_Elo";
const std::string MultiPvOption = "MultiPV";

const std::regex IdNameRegex("id name (.*)");
const std::regex InfoPvRegex("\\bmultipv (\\d+)\\b.*\\bscore (cp|mate) (-?\\d+)\\b.*\\bpv (\\S+)");
const std::regex OptionNameRegex("option name (.*) type (.*)");
const std::regex BestMoveRegex("bestmove (\\S+) ponder (\\S+)");
const std::regex BestMoveRegex1("bestmove (\\S+)");

// Write one UCI command line to the engine's stdin.
static void sendLine(QProcess* proc, const std::string& text)
{
	std::string msg = text + "\n";
	proc->write(msg.c_str(), static_cast<qint64>(msg.size()));
	proc->waitForBytesWritten(3000);
}

// Block until one complete line is available, then return it trimmed.
// Returns empty string on timeout or process exit.
static std::string readLineBlocking(QProcess* proc, int timeoutMs = 5000)
{
	while (!proc->canReadLine()) {
		if (proc->state() != QProcess::Running)
			return "";
		if (!proc->waitForReadyRead(timeoutMs))
			return "";
	}
	return QString(proc->readLine()).trimmed().toStdString();
}

UciConnector::UciConnector() : _initOk(false)
{
	_uciEngine = std::make_unique<QProcess>();
	_uciEngine->setReadChannel(QProcess::StandardOutput);
}

// The engine installed next to the app (./engines/) first, then one on the system PATH.
std::vector<std::string> UciConnector::EngineCandidates()
{
	const QString enginesDir = QCoreApplication::applicationDirPath() + "/engines";
	std::vector<std::string> paths;
	for (const QString& path : { QStandardPaths::findExecutable(UciEngineProgrammExe, { enginesDir }),
	                             QStandardPaths::findExecutable(UciEngineProgrammExe) }) {
		if (!path.isEmpty() && std::find(paths.begin(), paths.end(), path.toStdString()) == paths.end())
			paths.push_back(path.toStdString());
	}
	return paths;
}

void UciConnector::Init()
{
	// Init() is also the restart path after the engine exited (Stockfish 18+ quits on
	// an illegal move in "position ... moves"), so drop any previous process state.
	if (_uciEngine->state() != QProcess::NotRunning) {
		_uciEngine->kill();
		_uciEngine->waitForFinished(1000);
	}
	_opt.clear();
	_initOk = false;

	// A candidate that is found but fails to start (e.g. a corrupt download) falls through to the next one.
	const std::vector<std::string> candidates = EngineCandidates();
	const bool found = !candidates.empty();
	bool started = false;
	for (const std::string& sfPath : candidates) {
		_uciEngine->start(QString::fromStdString(sfPath), QStringList());
		if (_uciEngine->waitForStarted(5000)) {
			started = true;
			_enginePath = sfPath;
			Trace("-- engine started: " + _enginePath);
			break;
		}
		_uciEngine->kill();
		_uciEngine->waitForFinished(1000);
	}

	if (!found)
		throw std::runtime_error(
			"UciConnector: stockfish.exe not found in ./engines/ or on PATH");
	if (!started)
		throw std::runtime_error("UciConnector: failed to start stockfish");

	sendLine(_uciEngine.get(), UciInitCommand);

	while (_uciEngine->state() == QProcess::Running) {
		std::string line = readLineBlocking(_uciEngine.get(), 5000);

		if (line.find(UciOkCommand) != std::string::npos)
			break;

		std::cmatch m;
		if (std::regex_match(line.c_str(), m, IdNameRegex))
			_opt["id"] = m[1];
		else if (std::regex_match(line.c_str(), m, OptionNameRegex))
			_opt[m[1]] = m[2];
	}

	_initOk = CheckReady();
}

static void Log(const std::string& line)
{
#ifdef _DEBUG
	auto logFileName = "uci_debug_log.txt";
	auto now = std::chrono::system_clock::now();

	// Format directly into a std::string (HH:MM:SS format)
	std::string time_str = std::format("{:%H:%M:%S}", now);

	std::ofstream log(logFileName, std::ios::app);
	log << "[" << time_str << "] " << line << std::endl << std::flush;
#endif // DEBUG
}

std::string UciConnector::ProcessCommand(const Command& comm, std::vector<std::string>* lines)
{
	Log(std::format(">>> Processing command: '{0}', expecting response: '{1}'",
		comm.Request, comm.Response));

	Trace(">> " + comm.Request);
	sendLine(_uciEngine.get(), comm.Request);
	if (comm.Response.empty())
		return "";

	// A search prints an "info depth" line per depth (times MultiPV). Keep them out of the
	// short trace buffer, which must still hold the position/go commands when a request
	// fails; trace only how many there were and the last one.
	std::string line, lastSearchInfo;
	int searchInfoCount = 0;
	auto traceSearchInfo = [&] {
		if (searchInfoCount > 0)
			Trace(std::format("<< ({} search info lines, last:) {}", searchInfoCount, lastSearchInfo));
		searchInfoCount = 0;
	};

	while (_uciEngine->state() == QProcess::Running) {
		line = readLineBlocking(_uciEngine.get(), 10000);
		if (line.empty())
			break;
		if (line.rfind("info depth", 0) == 0) {
			lastSearchInfo = line;
			++searchInfoCount;
		}
		else {
			traceSearchInfo();
			Trace("<< " + line);
		}
		if (line.find(comm.Response) != std::string::npos)
			break;
		if (lines)
			lines->push_back(line);
	}
	traceSearchInfo();

	if (line.empty())
		Trace(_uciEngine->state() == QProcess::Running
			? "<< (no output within 10 s, engine still running)"
			: "<< (no output, engine process is not running)");

	Log(std::format("Engine response: '{}' <<<", line));
	return line;
}

void UciConnector::Trace(const std::string& line)
{
	constexpr size_t MaxLines = 30, MaxLineLength = 240;
	_traffic.push_back(line.size() > MaxLineLength ? line.substr(0, MaxLineLength) + "..." : line);
	if (_traffic.size() > MaxLines)
		_traffic.pop_front();
}

std::string UciConnector::DebugState()
{
	std::string out = "engine: " + (_enginePath.empty() ? std::string("(not started)") : _enginePath);
	out += " [" + _opt["id"] + "]\n";

	out += "process: ";
	switch (_uciEngine->state()) {
	case QProcess::Running:    out += "running"; break;
	case QProcess::Starting:   out += "starting"; break;
	default:
		out += std::format("not running, exit code {}, {}", _uciEngine->exitCode(),
			_uciEngine->exitStatus() == QProcess::CrashExit ? "crashed" : "exited normally");
	}
	out += ", last error: " + _uciEngine->errorString().toStdString() + "\n";

	auto err = QString::fromLocal8Bit(_uciEngine->readAllStandardError()).trimmed();
	if (!err.isEmpty())
		out += "stderr: " + err.toStdString() + "\n";

	out += "recent traffic (oldest first):\n";
	for (const auto& l : _traffic)
		out += "  " + l + "\n";
	return out;
}

bool UciConnector::IsInitialized()
{
	return _initOk;
}

bool UciConnector::IsRunning() const
{
	return _uciEngine && _uciEngine->state() == QProcess::Running;
}

bool UciConnector::CheckReady()
{
	Command isReady = { ReadyCommand, ReadyOkCommand };
	auto response = ProcessCommand(isReady);
	return response == ReadyOkCommand;
}

bool UciConnector::NewGame()
{
	ProcessCommand({ UciNewGameCommand, "" });
	return CheckReady();
}

void UciConnector::Kill()
{
	// Safe to call cross-thread: TerminateProcess() is an OS-level call.
	// Causes any blocking waitForReadyRead() in the engine thread to return,
	// allowing the thread to exit cleanly before we destroy it.
	if (_uciEngine && _uciEngine->state() != QProcess::NotRunning)
		_uciEngine->kill();
}

UciConnector::~UciConnector()
{
	if (_uciEngine && _uciEngine->state() == QProcess::Running) {
		sendLine(_uciEngine.get(), QuitCommand);
		if (!_uciEngine->waitForFinished(3000)) {
			_uciEngine->kill();
			_uciEngine->waitForFinished(1000);
		}
	}
}

std::string UciConnector::GetOption(const std::string& op)
{
	// find(), not operator[]: inserting the key would make SetOption() accept an option
	// the engine doesn't have.
	auto it = _opt.find(op);
	return it == _opt.end() ? std::string() : it->second;
}

void UciConnector::SetOption(const std::string& op, const std::string& value)
{
	if (_opt.find(op) == _opt.end())
		throw std::invalid_argument("Unknown option: '" + op + "'");

	std::string cmd = "setoption name " + op + " value " + value;
	ProcessCommand({ cmd, "" });
	if (!CheckReady())
		throw std::logic_error("Setting: '" + op + "' failed!");

	_opt[op] = value;
}

void UciConnector::SetDifficulty(int elo)
{
	// Skill Level alone barely affects Stockfish's actual strength; UCI_LimitStrength
	// + UCI_Elo is the documented way to make the engine genuinely play weaker.
	elo = std::max(1320, std::min(3190, elo));
	SetOption(LimitStrengthOption, "true");
	SetOption(EloOption, std::to_string(elo));
}

std::vector<std::string> UciConnector::GetOptions()
{
	std::vector<std::string> res;
	for (const auto& x : _opt)
		res.push_back(x.first);
	return res;
}

static std::string PositionCommand(const StartPosMoveRequest& req)
{
	std::string posCmd = req.Fen.empty()
		? "position startpos"
		: "position fen " + req.Fen;
	if (!req.Moves.empty()) {
		std::string moves;
		for (const auto& move : req.Moves) moves += move + " ";
		posCmd += " moves " + moves;
	}
	return posCmd;
}

static EngineMoveResponse ParseBestMove(const std::string& resp)
{
	std::cmatch m;
	if (std::regex_match(resp.c_str(), m, BestMoveRegex))
		return { m[1], m[2] };

	if (std::regex_match(resp.c_str(), m, BestMoveRegex1))
		return { m[1], {} };

	//c7c8q for a Queen or c7c8n for a Knight)

	throw std::logic_error("Engine move failed, response: '" + resp + "'");
}

EngineMoveResponse UciConnector::GetEngineMove(const StartPosMoveRequest& req,
	std::chrono::milliseconds moveTime)
{
	ProcessCommand({ PositionCommand(req), "" });

	auto moveMs = static_cast<int>(moveTime.count());
	return ParseBestMove(ProcessCommand({ "go movetime " + std::to_string(moveMs), BestMoveCommand }));
}

std::vector<EngineCandidate> UciConnector::GetCandidateMoves(const StartPosMoveRequest& req,
	int count, int depth)
{
	// SetOption() throws for an engine without MultiPV, rather than silently returning a
	// single (best) candidate.
	if (GetOption(MultiPvOption) != std::to_string(count))
		SetOption(MultiPvOption, std::to_string(count));

	ProcessCommand({ PositionCommand(req), "" });

	std::vector<std::string> info;
	auto best = ParseBestMove(ProcessCommand({ "go depth " + std::to_string(depth), BestMoveCommand }, &info));

	// Each depth reports every MultiPV line again; later lines overwrite earlier ones,
	// so what is left is the deepest result for each rank.
	std::map<int, EngineCandidate> byRank;
	std::smatch m;
	for (const auto& line : info) {
		if (line.find("lowerbound") != std::string::npos || line.find("upperbound") != std::string::npos)
			continue; // aspiration-window result, not a final score
		if (!std::regex_search(line, m, InfoPvRegex))
			continue;
		int score = std::stoi(m[3].str());
		if (m[2] == "mate")
			score = score > 0 ? MateScoreCp - score : -MateScoreCp - score;
		byRank[std::stoi(m[1].str())] = { m[4].str(), score };
	}

	std::vector<EngineCandidate> res;
	for (const auto& [rank, c] : byRank)
		res.push_back(c);
	if (res.empty())
		res.push_back({ best.BestMove, 0 });
	return res;
}

std::string PickWeakMove(const std::vector<EngineCandidate>& candidates, int temperatureCp,
	std::mt19937& rng)
{
	if (candidates.empty())
		throw std::invalid_argument("PickWeakMove: no candidate moves");

	auto isMate = [](int cp) { return std::abs(cp) > MateScoreCp / 2; };

	// Ordinary scores are used as they are, so even far ahead the picker still prefers the
	// moves that win fastest. Mate scores are replaced by "the best ordinary move plus a
	// margin" (or "the worst minus a margin" when being mated), so a mate is very likely but
	// not certain to be found; shorter mates rank slightly higher.
	static constexpr int MateMarginCp = 600, PerMoveCp = 10;
	int bestCp = INT_MIN, worstCp = INT_MAX;
	for (const auto& c : candidates) {
		if (isMate(c.ScoreCp))
			continue;
		bestCp = std::max(bestCp, c.ScoreCp);
		worstCp = std::min(worstCp, c.ScoreCp);
	}
	if (bestCp == INT_MIN)
		bestCp = worstCp = 0; // every candidate is a mate

	auto effective = [&](int cp) {
		if (!isMate(cp))
			return cp;
		int moves = MateScoreCp - std::abs(cp);
		return cp > 0 ? bestCp + MateMarginCp - PerMoveCp * moves
		              : worstCp - MateMarginCp + PerMoveCp * moves;
	};

	int top = INT_MIN;
	for (const auto& c : candidates)
		top = std::max(top, effective(c.ScoreCp));

	std::vector<double> weights;
	for (const auto& c : candidates)
		weights.push_back(std::exp(-double(top - effective(c.ScoreCp)) / std::max(1, temperatureCp)));

	std::discrete_distribution<size_t> pick(weights.begin(), weights.end());
	return candidates[pick(rng)].Move;
}
