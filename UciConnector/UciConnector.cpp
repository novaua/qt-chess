// UciConnector.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "UciConnector.h"

const std::string UciEngineProgramm = "E:\\Tools\\bin\\stockfish.exe";

const std::string UciInitCommand = "uci";
const std::string UciOkCommand = "uciok";
const std::string UciNewGameCommand = "ucinewgame";

const std::string ReadyCommand = "isready";
const std::string ReadyOkCommand = "readyok";

const std::string QuitCommand = "quit";

/// <summary>
/// Options and IDs
/// </summary>
const std::regex IdNameRegex("id name (.*)");
const std::regex OptionNameRegex("option name (.*) type (.*)");

UciConnector::UciConnector() {
}

void UciConnector::Init() {
	_uciEngine = bp::child(bp::search_path(UciEngineProgramm), bp::std_out > out, bp::std_in < in);

	in << UciInitCommand << std::endl;

	std::string line;
	while (_uciEngine.running() && std::getline(out, line) && line.find(UciOkCommand) == std::string::npos)
	{
		//std::cout << "id " << line << std::endl << std::flush;
		line = boost::algorithm::trim_copy(line);

		std::cmatch what;
		if (std::regex_match(line.c_str(), what, IdNameRegex)) {
			//std::cout << "id " << what[1] << std::endl << std::flush;
			_opt["id"] = what[1];
		}
		else if (std::regex_match(line.c_str(), what, OptionNameRegex)) {
			//std::cout << "option " << what[1] << " | " << what[2] << std::endl << std::flush;
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