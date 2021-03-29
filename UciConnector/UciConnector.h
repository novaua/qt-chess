#pragma once
#include <string>
#include <boost/process.hpp>

namespace bp = boost::process;

struct Command {

	std::string Request;

	std::string Response;
};

class UciConnector
{
	bool _initOk;
	bp::child _uciEngine;
	bp::opstream in;
	bp::ipstream out;

	std::map<std::string, std::string> _opt;
public:

	UciConnector();

	void Init();

	bool IsInitialized();

	bool CheckReady();
	bool NewGame();

	std::string GetOption(const std::string& op);
	void SetOption(const std::string& op, const std::string& value);
	std::vector<std::string> GetOptions();

	std::string ProcessCommand(const Command& comm);

	~UciConnector();
};

