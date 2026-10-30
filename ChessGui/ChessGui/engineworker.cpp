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
    ENGINE_TRACE("worker: request" << request << "started," << history.size() << "plies, thread" << QThread::currentThreadId());
    try {
        if (!_enginePlayer)
            _enginePlayer = std::make_shared<Chess::ChessEnginePlayer>(_level, _mode);
        const auto move = QString::fromStdString(_enginePlayer->FindMove(history));
        ENGINE_TRACE("worker: request" << request << "answers" << move);
        emit moveFound(request, move);
    } catch (const std::exception& e) {
        ENGINE_TRACE("worker: request" << request << "failed:" << e.what());
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
