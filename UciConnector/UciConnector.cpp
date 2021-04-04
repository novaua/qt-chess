// UciConnector.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "UciConnector.h"
#include <chrono>

const std::string UciEngineProgramm = "E:\\Tools\\bin\\stockfish.exe";

const std::string UciInitCommand = "uci";
const std::string UciOkCommand = "uciok";
const std::string UciNewGameCommand = "ucinewgame";

const std::string ReadyCommand = "isready";
const std::string ReadyOkCommand = "readyok";
const std::string BestMoveCommand = "bestmove";

const std::string QuitCommand = "quit";


/// <summary>
/// Options and IDs
/// </summary>
const std::regex IdNameRegex("id name (.*)");
const std::regex OptionNameRegex("option name (.*) type (.*)");
const std::regex BestMoveRegex("bestmove (.*) ponder (.*)");

UciConnector::UciConnector() {
}

void UciConnector::Init() {
	_uciEngine = bp::child(bp::search_path(UciEngineProgramm), bp::std_out > out, bp::std_in < in);
	in << UciInitCommand << std::endl;

	std::string line;
	while (_uciEngine.running() && std::getline(out, line) && line.find(UciOkCommand) == std::string::npos)
	{
		line = boost::algorithm::trim_copy(line);

		std::cmatch what;
		if (std::regex_match(line.c_str(), what, IdNameRegex)) {
			_opt["id"] = what[1];
		}
		else if (std::regex_match(line.c_str(), what, OptionNameRegex)) {
			_opt[what[1]] = what[2];
		}
	}

	_initOk = CheckReady();
}

std::string UciConnector::ProcessCommand(const Command& comm) {

	in << comm.Request << std::endl;

	if (comm.Response.empty()) {
		return "";
	}

	std::string line;
	while (_uciEngine.running() && std::getline(out, line) && line.find(comm.Response) == std::string::npos)
	{
		std::cout << line << std::endl << std::flush;
	}

	return boost::algorithm::trim_copy(line);
}

bool UciConnector::IsInitialized() {
	return _initOk;
}

bool UciConnector::CheckReady() {
	Command isReady = { ReadyCommand, ReadyOkCommand };
	auto response = ProcessCommand(isReady);
	return response == ReadyOkCommand;
}

bool UciConnector::NewGame() {
	ProcessCommand({ UciNewGameCommand });
	return CheckReady();
}

UciConnector::~UciConnector() {
	in << QuitCommand << std::endl;
	if (!_uciEngine.wait_for(std::chrono::seconds(3))) {
		_uciEngine.terminate();
	}
}

std::string UciConnector::GetOption(const std::string& op) {
	return _opt[op];
}

void UciConnector::SetOption(const std::string& op, const std::string& value)
{
	if (_opt.find(op) == _opt.end())
	{
		throw std::invalid_argument("Unknown option: '" + op + "'");
	}

	auto setOptCommand = (boost::format("setoption name %1% value %2%") % op % value).str();
	ProcessCommand({ setOptCommand,"" });
	if (!CheckReady())
		throw std::logic_error("Setting: '" + op + "' failed!");
}

std::vector<std::string> UciConnector::GetOptions()
{
	std::vector<std::string> res;
	for (auto const& x : _opt)
	{
		res.push_back(x.first);
	}

	return res;
}

EngineMoveResponse UciConnector::GetEngineMove(const StartPosMoveRequest& req, const std::chrono::seconds& moveTime)
{
	std::string moves;
	for (auto move : req.Moves) {
		moves += move + " ";
	}

	ProcessCommand({ (boost::format("position startpos moves %1%") % moves).str() });

	std::stringstream goRequest;
	goRequest << "go movetime " << std::chrono::milliseconds(moveTime).count();

	Command goCommand = { goRequest.str(), BestMoveCommand };

	auto responseStr = ProcessCommand(goCommand);

	std::cmatch what;
	if (std::regex_match(responseStr.c_str(), what, BestMoveRegex)) {
		return { what[1], what[2] };
	}

	throw std::logic_error("Getting engine move failed! request: '" + goRequest.str() +
		"' response: '" + responseStr + "'");
}
