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
public:

	UciConnector();

	void Init();

	bool IsInitialized();

	bool CheckReady();

	Command ProcessCommand(const Command& comm);

	~UciConnector();
};

