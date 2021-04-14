#pragma once
#include <string>
#include <boost/process.hpp>
#include <memory>
#include <chrono>

namespace bp = boost::process;

struct Command
{
	std::string Request;

	std::string Response;
};

struct StartPosMoveRequest
{
	std::vector<std::string> Moves; // e2e4 e7e5 b1c3
};

struct EngineMoveResponse
{
	std::string BestMove;

	std::string Ponder;
};

class UciConnector
{
	bool _initOk;
	bp::child _uciEngine;
	bp::opstream in;
	bp::ipstream out;

	std::map<std::string, std::string> _opt;

protected:

	std::string ProcessCommand(const Command& comm);

public:

	UciConnector();

	void Init();

	bool IsInitialized();

	bool CheckReady();
	bool NewGame();

	std::string GetOption(const std::string& op);
	void SetOption(const std::string& op, const std::string& value);
	std::vector<std::string> GetOptions();

	EngineMoveResponse GetEngineMove(const StartPosMoveRequest& req, const std::chrono::seconds& moveTime);

	~UciConnector();
};

typedef std::shared_ptr<UciConnector> UciConnectorAPtr;

