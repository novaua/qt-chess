// UciConnector.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "framework.h"
#include "UciConnector.h"
#include<boost/algorithm/string.hpp>

const std::string UciEngineProgramm = "E:\\Tools\\bin\\stockfish.exe";

const std::string UciInitCommand = "uci";
const std::string UciOkCommand = "uciok";

const std::string ReadyCommand = "isready";
const std::string ReadyOkCommand = "readyok";

const std::string QuitCommand = "quit";

UciConnector::UciConnector() {
}

void UciConnector::Init() {
	_uciEngine = bp::child(bp::search_path(UciEngineProgramm), bp::std_out > out, bp::std_in < in);

	in << UciInitCommand << std::endl;

	std::string line;
	while (_uciEngine.running() && std::getline(out, line) && line.find(UciOkCommand) == std::string::npos)
	{
		std::cout << line << std::endl << std::flush;
	}

	_initOk = CheckReady();
}

Command UciConnector::ProcessCommand(const Command& comm) {

	in << comm.Request << std::endl;

	std::string line;
	while (_uciEngine.running() && std::getline(out, line) && line.find(comm.Response) == std::string::npos)
	{
		std::cout << line << std::endl << std::flush;
	}

	return { comm.Request, boost::algorithm::trim_copy(line) };
}

bool UciConnector::IsInitialized() {
	return _initOk;
}

bool UciConnector::CheckReady() {
	Command isReady = { ReadyCommand, ReadyOkCommand };
	auto response = ProcessCommand(isReady);
	return response.Response == ReadyOkCommand;
}

UciConnector::~UciConnector() {
	in << QuitCommand << std::endl;
	if (!_uciEngine.wait_for(std::chrono::seconds(3))) {
		_uciEngine.terminate();
	}
}
