#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <vector>

class QProcess;

struct Command
{
	std::string Request;
	std::string Response;
};

struct StartPosMoveRequest
{
	std::vector<std::string> Moves;
};

struct EngineMoveResponse
{
	std::string BestMove;
	std::string Ponder;
};

class UciConnector
{
	bool _initOk = false;
	std::unique_ptr<QProcess> _uciEngine;
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
	EngineMoveResponse GetEngineMove(const StartPosMoveRequest& req,
	                                 std::chrono::milliseconds moveTime);
	void SetDifficulty(int level);
	void Kill();
	~UciConnector();
};

typedef std::shared_ptr<UciConnector> UciConnectorAPtr;
