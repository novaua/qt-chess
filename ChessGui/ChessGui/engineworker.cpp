#include "stdafx.h"
#include "engineworker.h"

EngineWorker::EngineWorker(Chess::EngineLevel level, Chess::PositionMode mode, QObject* parent)
    : QObject(parent), _level(level), _mode(mode)
{}

// Called from the GUI thread to unblock a waiting findMove(): killing the process is
// an OS-level call, safe across threads.
void EngineWorker::killEngine()
{
    if (_enginePlayer)
        _enginePlayer->KillEngine();
}

// Runs on the engine thread. ChessEnginePlayer (and its QProcess) are created here
// so they are owned by and always accessed from the engine thread.
void EngineWorker::findMove(int request, const Chess::MovesHistory& history)
{
    try {
        if (!_enginePlayer)
            _enginePlayer = std::make_shared<Chess::ChessEnginePlayer>(_level, _mode);
        emit moveFound(request, QString::fromStdString(_enginePlayer->FindMove(history)));
    } catch (const std::exception& e) {
        QString details;
        try {
            if (_enginePlayer)
                details = QString::fromStdString(_enginePlayer->DebugDump(history));
        } catch (const std::exception& dumpError) {
            details = QString("(debug dump failed: %1)").arg(dumpError.what());
        }
        emit moveError(request, QString::fromStdString(e.what()), details);
    }
}
