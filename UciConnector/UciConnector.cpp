// UciConnector.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "UciConnector.h"
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

const QString UciEngineProgrammExe = "stockfish.exe";

const std::string UciInitCommand    = "uci";
const std::string UciOkCommand      = "uciok";
const std::string UciNewGameCommand = "ucinewgame";
const std::string ReadyCommand      = "isready";
const std::string ReadyOkCommand    = "readyok";
const std::string BestMoveCommand   = "bestmove";
const std::string QuitCommand       = "quit";
const std::string SkillLevelOption  = "Skill Level";

const std::regex IdNameRegex    ("id name (.*)");
const std::regex OptionNameRegex("option name (.*) type (.*)");
const std::regex BestMoveRegex  ("bestmove (\\S+) ponder (\\S+)");

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

void UciConnector::Init()
{
    QString sfPath = QStandardPaths::findExecutable(UciEngineProgrammExe);
    if (sfPath.isEmpty()) {
        QString enginesDir = QCoreApplication::applicationDirPath() + "/engines";
        sfPath = QStandardPaths::findExecutable(UciEngineProgrammExe, {enginesDir});
    }
    if (sfPath.isEmpty())
        throw std::runtime_error(
            "UciConnector: stockfish.exe not found on PATH or in ./engines/");

    _uciEngine->start(sfPath, QStringList());
    if (!_uciEngine->waitForStarted(5000))
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

std::string UciConnector::ProcessCommand(const Command& comm)
{
    sendLine(_uciEngine.get(), comm.Request);
    if (comm.Response.empty()) return "";

    std::string line;
    while (_uciEngine->state() == QProcess::Running) {
        line = readLineBlocking(_uciEngine.get(), 10000);
        if (line.empty()) 
            break;
        if (line.find(comm.Response) != std::string::npos) 
            break;
		// ToDo: Add logging of engine output, with a way to enable/disable it.
        // std::cout << line << std::endl << std::flush;
    }
    return line;
}

bool UciConnector::IsInitialized()
{
    return _initOk;
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
    return _opt[op];
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

void UciConnector::SetDifficulty(int level)
{
    level = std::max(0, std::min(20, level));
    SetOption(SkillLevelOption, std::to_string(level));
}

std::vector<std::string> UciConnector::GetOptions()
{
    std::vector<std::string> res;
    for (const auto& x : _opt)
        res.push_back(x.first);
    return res;
}

EngineMoveResponse UciConnector::GetEngineMove(const StartPosMoveRequest& req,
                                               std::chrono::milliseconds moveTime)
{
    std::string moves;
    for (const auto& move : req.Moves)
        moves += move + " ";
    ProcessCommand({ "position startpos moves " + moves, "" });

    auto moveMs = static_cast<int>(moveTime.count());
    std::string goCmd = "go movetime " + std::to_string(moveMs);
    sendLine(_uciEngine.get(), goCmd);

	// Per-line timeout: moveTime + 10s buffer.
    // Stockfish emits many "info depth" lines before "bestmove", each within the
    // search window, so the budget must be at least as large as the search time.
    int budget = moveMs + 10000;
    std::string resp;
    while (_uciEngine->state() == QProcess::Running) {
        resp = readLineBlocking(_uciEngine.get(), budget);
        if (resp.empty()) break;
        if (resp.find(BestMoveCommand) != std::string::npos) break;
    }

    std::cmatch m;
    if (std::regex_match(resp.c_str(), m, BestMoveRegex))
        return { m[1], m[2] };

    throw std::logic_error("Engine move failed, response: '" + resp + "'");
}
